/****************************************************************************
 * Contest 2026 team 069 - Fall Detect AI Agent Application
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * AI Agent Skill Registration
 *
 * This application initializes the AI Agent framework and registers
 * three skills for fall detection, patrol, and voice help.
 ****************************************************************************/

/* Skill entry points (provided by ai_agent library) */
extern int ai_agent_init(void);
extern int ai_agent_register_skill(const char *name, const char *desc);
extern int ai_agent_start_loop(void);
extern void ai_agent_deinit(void);

/* Skill registration table */
struct skill_entry
{
  const char *name;
  const char *desc;
};

static const struct skill_entry g_skills[] =
{
  { "fall-detect",  "Real-time fall detection via accelerometer" },
  { "fall-patrol",  "Post-fall patrol: camera capture + LLM analysis + Feishu notify" },
  { "voice-help",   "TTS voice alert and microphone listening for user response" },
};

#define NUM_SKILLS (sizeof(g_skills) / sizeof(g_skills[0]))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int ret;
  int i;

  printf("[fall_detect_app] Starting AI Agent for fall detection...\n");

  /* Step 1: Initialize AI Agent framework */

  ret = ai_agent_init();
  if (ret < 0)
    {
      fprintf(stderr, "[fall_detect_app] ai_agent_init failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  printf("[fall_detect_app] AI Agent initialized.\n");

  /* Step 2: Register skills */

  for (i = 0; i < (int)NUM_SKILLS; i++)
    {
      ret = ai_agent_register_skill(g_skills[i].name, g_skills[i].desc);
      if (ret < 0)
        {
          fprintf(stderr, "[fall_detect_app] Failed to register skill '%s': %d\n",
                  g_skills[i].name, ret);
          ai_agent_deinit();
          return EXIT_FAILURE;
        }

      printf("[fall_detect_app] Registered skill: %s\n", g_skills[i].name);
    }

  printf("[fall_detect_app] All %d skills registered. Entering main loop.\n",
         (int)NUM_SKILLS);

  /* Step 3: Enter main event loop (blocks until shutdown) */

  ret = ai_agent_start_loop();
  if (ret < 0)
    {
      fprintf(stderr, "[fall_detect_app] Main loop exited with error: %d\n", ret);
    }

  /* Step 4: Cleanup */

  ai_agent_deinit();
  printf("[fall_detect_app] AI Agent shut down.\n");

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
