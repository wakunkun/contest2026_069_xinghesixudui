# Contest 2026 team 069 fall_detect_app
if(CONFIG_LVX_USE_DEMO_CONTEST2026_069_FALL_DETECT_APP)
  nuttx_add_application(
    NAME fall_detect_app
    SRCS fall_detect_main.c
    STACKSIZE 4096)
endif()
