if(CONFIG_BOARD_KAUSA_LEFT OR CONFIG_BOARD_KAUSA_RIGHT)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
