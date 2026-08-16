/****************************************************************************
 * Contest 2026 team 069 - Silver Guardian runtime entry point
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <nuttx/sensors/qma7981.h>

#include "core/message_bus.h"

#ifndef CONFIG_EXAMPLES_AI_AGENT_VELA_DATA_DIR
#  define CONFIG_EXAMPLES_AI_AGENT_VELA_DATA_DIR "/data/ai_agent"
#endif

#define CONTEST_SKILLS_DIR \
  CONFIG_EXAMPLES_AI_AGENT_VELA_DATA_DIR "/skills"

#define ACCEL_SAMPLE_BYTES ((ssize_t)sizeof(struct qma7981_data_s))
#define ACCEL_THRESHOLD_RAW 1536 /* 3 g @ ±4 g range (512 LSB/g, measured 1g≈ 512) */
#define ACCEL_THRESHOLD_SQ ((int64_t)ACCEL_THRESHOLD_RAW * ACCEL_THRESHOLD_RAW)
#define FALL_DEBOUNCE_MS 500
#define SENSOR_POLL_DELAY_US 100000

extern int ai_agent_main(int argc, char *argv[]);
extern void agent_request_shutdown(void);
extern bool agent_shutdown_requested(void);

struct contest_skill
{
  const char *name;
  const char *content;
};

struct runtime_state
{
  volatile bool agent_finished;
  int agent_result;
};

static const struct contest_skill g_contest_skills[] =
{
  {
    "fall-detect",
    "# fall-detect Skill\n"
    "\n"
    "Monitor the QMA7981 accelerometer and identify a possible fall.\n"
    "\n"
    "## Trigger\n"
    "- Treat a resultant acceleration above 3 g as a suspected impact.\n"
    "- Apply a 500 ms debounce window before raising another event.\n"
    "\n"
    "## Flow\n"
    "1. Read the accelerometer sample and record the timestamp.\n"
    "2. Publish a fall_suspected event with the raw X/Y/Z sample.\n"
    "3. Ask fall-patrol to capture a photo and analyze the scene.\n"
    "4. Ask voice-help to provide a local safety prompt.\n"
    "\n"
    "## Safety\n"
    "A sensor event is only a suspicion. Use the camera result and user response "
    "before escalating to an emergency notification.\n"
  },
  {
    "fall-patrol",
    "# fall-patrol Skill\n"
    "\n"
    "Handle a suspected fall and perform a short visual patrol.\n"
    "\n"
    "## Flow\n"
    "1. Use camera_capture for the configured camera device.\n"
    "2. Use analyze_image on the captured image when a vision model is configured.\n"
    "3. Combine the image result with the accelerometer event.\n"
    "4. Use Feishu tools to notify the configured caregiver when escalation is needed.\n"
    "\n"
    "## Outputs\n"
    "Report photo_captured, llm_analysis_done, and feishu_notification_sent states.\n"
    "If the camera, network, or model is unavailable, keep the sensor event and "
    "report the degraded mode instead of claiming success.\n"
  },
  {
    "voice-help",
    "# voice-help Skill\n"
    "\n"
    "Provide a local voice warning after a suspected fall and listen for a response.\n"
    "\n"
    "## Flow\n"
    "1. Announce a short Chinese safety prompt when audio output is available.\n"
    "2. Listen for a response for up to five seconds when microphone input is available.\n"
    "3. Treat an emergency keyword or no response as an escalation signal.\n"
    "4. Report voice_alert_sent, user_response_received, or emergency_escalated.\n"
    "\n"
    "Never claim that audio was played or heard when the corresponding hardware or "
    "service is unavailable.\n"
  }
};

static int ensure_directory_tree(const char *path)
{
  char buffer[256];
  size_t length;
  char *cursor;

  if (path == NULL)
    {
      return -EINVAL;
    }

  length = strlen(path);
  if (length == 0 || length >= sizeof(buffer))
    {
      return -ENAMETOOLONG;
    }

  memcpy(buffer, path, length + 1);

  for (cursor = buffer + 1; *cursor != '\0'; cursor++)
    {
      if (*cursor == '/')
        {
          *cursor = '\0';
          if (mkdir(buffer, 0755) < 0 && errno != EEXIST)
            {
              return -errno;
            }
          *cursor = '/';
        }
    }

  if (mkdir(buffer, 0755) < 0 && errno != EEXIST)
    {
      return -errno;
    }

  return 0;
}

static int seed_skill(const struct contest_skill *skill)
{
  char path[256];
  FILE *file;
  size_t length;

  if (snprintf(path, sizeof(path), "%s/%s.md", CONTEST_SKILLS_DIR,
               skill->name) >= (int)sizeof(path))
    {
      return -ENAMETOOLONG;
    }

  if (access(path, F_OK) == 0)
    {
      return 0;
    }

  file = fopen(path, "w");
  if (file == NULL)
    {
      return -errno;
    }

  length = strlen(skill->content);
  if (fwrite(skill->content, 1, length, file) != length || fclose(file) != 0)
    {
      return -EIO;
    }

  printf("[fall_detect_app] seeded skill: %s\n", skill->name);
  return 0;
}

static int seed_contest_skills(void)
{
  size_t index;
  int ret;

  ret = ensure_directory_tree(CONTEST_SKILLS_DIR);
  if (ret < 0)
    {
      return ret;
    }

  for (index = 0; index < sizeof(g_contest_skills) / sizeof(g_contest_skills[0]);
       index++)
    {
      ret = seed_skill(&g_contest_skills[index]);
      if (ret < 0)
        {
          fprintf(stderr, "[fall_detect_app] failed to seed %s: %d\n",
                  g_contest_skills[index].name, ret);
          return ret;
        }
    }

  return 0;
}

static int publish_fall_event(const struct qma7981_data_s *sample)
{
  char content[256];
  agent_msg_t message;

  if (snprintf(content, sizeof(content),
               "fall_suspected: accelerometer impact detected; "
               "raw_x=%d raw_y=%d raw_z=%d. Run fall-patrol for camera "
               "confirmation and voice-help for a local safety prompt.",
               sample->x, sample->y, sample->z) >= (int)sizeof(content))
    {
      return -ENAMETOOLONG;
    }

  memset(&message, 0, sizeof(message));
  strncpy(message.channel, "system", sizeof(message.channel) - 1);
  strncpy(message.chat_id, "fall-detect", sizeof(message.chat_id) - 1);
  message.content = strdup(content);
  if (message.content == NULL)
    {
      return -ENOMEM;
    }

  if (message_bus_push_inbound(&message) < 0)
    {
      free(message.content);
      return -EAGAIN;
    }

  printf("[fall_detect_app] fall_suspected published: %d,%d,%d\n",
         sample->x, sample->y, sample->z);
  return 0;
}

static void *run_ai_agent(void *arg)
{
  struct runtime_state *state = (struct runtime_state *)arg;
  char *agent_argv[] = { (char *)"fall_detect_app", NULL };

  state->agent_result = ai_agent_main(1, agent_argv);
  state->agent_finished = true;
  return NULL;
}

static int monitor_accelerometer(struct runtime_state *state)
{
  struct qma7981_data_s sample;
  int fd;
  int64_t magnitude_sq;
  uint32_t debounce_us = 0;
  uint32_t sample_count = 0;

  fd = open("/dev/accel0", O_RDONLY);
  if (fd < 0)
    {
      printf("[fall_detect_app] /dev/accel0 unavailable: %d; "
             "agent remains available in degraded mode\n", errno);
      while (!state->agent_finished && !agent_shutdown_requested())
        {
          usleep(500000);
        }
      return 0;
    }

  printf("[fall_detect_app] monitoring /dev/accel0 (threshold=3g)\n");
  while (!state->agent_finished && !agent_shutdown_requested())
    {
      if (read(fd, &sample, sizeof(sample)) != ACCEL_SAMPLE_BYTES)
        {
          usleep(SENSOR_POLL_DELAY_US);
          continue;
        }

      if ((++sample_count % 50) == 0)
        {
          printf("[fall_detect_app] sample: x=%d y=%d z=%d\n",
                 sample.x, sample.y, sample.z);
        }

      magnitude_sq = (int64_t)sample.x * sample.x +
                     (int64_t)sample.y * sample.y +
                     (int64_t)sample.z * sample.z;

      if (debounce_us > 0)
        {
          debounce_us = debounce_us > SENSOR_POLL_DELAY_US
                        ? debounce_us - SENSOR_POLL_DELAY_US : 0;
        }
      else if (magnitude_sq >= ACCEL_THRESHOLD_SQ)
        {
          publish_fall_event(&sample);
          debounce_us = FALL_DEBOUNCE_MS * 1000;
        }

      usleep(SENSOR_POLL_DELAY_US);
    }

  close(fd);
  return 0;
}

int main(int argc, char *argv[])
{
  struct runtime_state state;
  pthread_t agent_thread;
  int ret;

  (void)argc;
  (void)argv;
  memset(&state, 0, sizeof(state));

  printf("[fall_detect_app] starting Silver Guardian runtime\n");

  ret = seed_contest_skills();
  if (ret < 0)
    {
      fprintf(stderr, "[fall_detect_app] skill provisioning failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = message_bus_init();
  if (ret < 0)
    {
      fprintf(stderr, "[fall_detect_app] message bus init failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = pthread_create(&agent_thread, NULL, run_ai_agent, &state);
  if (ret != 0)
    {
      fprintf(stderr, "[fall_detect_app] agent thread creation failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  monitor_accelerometer(&state);
  agent_request_shutdown();
  pthread_join(agent_thread, NULL);

  printf("[fall_detect_app] runtime stopped with result %d\n",
         state.agent_result);
  return state.agent_result < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
