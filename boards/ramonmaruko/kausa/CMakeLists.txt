if(CONFIG_BOARD_KAUSA_LEFT_NRF54L15_CPUAPP_ZMK OR CONFIG_BOARD_KAUSA_RIGHT_NRF54L15_CPUAPP_ZMK)
  zephyr_library()
  zephyr_library_sources(board_zmk.c)
endif()
