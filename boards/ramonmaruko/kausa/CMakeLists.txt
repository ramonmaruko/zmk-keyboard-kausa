if(CONFIG_BOARD_KAUSA_LEFT_NRF54L15_CPUAPP_ZMK OR CONFIG_BOARD_KAUSA_RIGHT_NRF54L15_CPUAPP_ZMK)
  zephyr_library()
  zephyr_library_sources(fuel_gauge.c)
  zephyr_library_sources(board_zmk.c)

  if(CONFIG_SETTINGS_ZMS)
    zephyr_library_sources(reset_settings_zms.c)
  endif()
endif()
