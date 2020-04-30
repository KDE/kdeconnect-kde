ki18n_wrap_ui(telhandler_SRCS dialog.ui)

add_executable(kdeconnect-handler kdeconnect-handler.cpp ${telhandler_SRCS})

target_include_directories(kdeconnect-handler PUBLIC ${CMAKE_BINARY_DIR})

target_link_libraries(kdeconnect-handler
    kdeconnectinterfaces
    kdeconnectcore
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
)

install(TARGETS kdeconnect-handler ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.handler.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
