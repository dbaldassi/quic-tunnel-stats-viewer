
qt_add_executable( ${exe}
    main.cpp
    main_window.h
    main_window.cpp
    main_window.ui
    received_bitrate_display.h
    received_bitrate_display.cpp
    csv_reader.h
    )

target_link_libraries( ${exe} PUBLIC
  Qt6::Widgets
  Qt6::OpenGL
  Qt6::Charts
  Qt6::Core
  )

set_target_properties( ${exe}
  PROPERTIES CXX_STANDARD 20
  )
