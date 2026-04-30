if(CONFIG_TAMAGOTCHI_WIDGET)
  target_sources(app PRIVATE
    src/tamagotchi_widget.c
    src/status_screen.c
  )
  target_include_directories(app PRIVATE src)
endif()
