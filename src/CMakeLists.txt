
qt_add_executable( ${exe}
    main.cpp
    main_window.h
    main_window.cpp
    main_window.ui
    received_bitrate_display.h
    received_bitrate_display.cpp
    csv_reader.h
    stats_line_chart.h stats_line_chart.cpp
    medooze_display.h medooze_display.cpp
    qlog_display.h qlog_display.cpp
    display_base.h display_base.cpp
    sent_loss_display.h sent_loss_display.cpp
    all_bitrate.h all_bitrate.cpp
    )

target_link_libraries( ${exe} PUBLIC
  Qt6::Widgets
  Qt6::OpenGL
  Qt6::Charts
  Qt6::Core

  nlohmann_json
  )

set_target_properties( ${exe}
  PROPERTIES CXX_STANDARD 23
  )
