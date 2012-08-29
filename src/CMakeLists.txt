include_directories(${FCITX4_FCITX_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_UTILS_INCLUDE_DIRS})
include_directories(${PROJECT_BINARY_DIR})

set(fcitx_pylogger_sources
  eim.c
  )

fcitx_add_addon(fcitx-pylogger ${fcitx_pylogger_sources})
