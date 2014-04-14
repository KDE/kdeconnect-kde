include_directories(${CMAKE_SOURCE_DIR})

add_executable(kdeconnect-cli kdeconnect-cli.cpp)

target_link_libraries(kdeconnect-cli kdeconnect ${QT_QTGUI_LIBRARY} ${KDE4_KDEUI_LIBS})

install(TARGETS kdeconnect-cli ${INSTALL_TARGETS_DEFAULT_ARGS})
