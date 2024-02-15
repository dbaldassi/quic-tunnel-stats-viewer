cmake_minimum_required( VERSION 3.16 FATAL_ERROR )

project( quic-tunnel-stats-viewer VERSION 1.0.0 )

# find_package( qt6_module )
find_package( Qt6 REQUIRED COMPONENTS Core Widgets Charts OpenGL )
qt_standard_project_setup()

set( exe stats_viewer )

add_subdirectory( src )
