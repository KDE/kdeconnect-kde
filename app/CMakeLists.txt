qt5_add_resources(kdeconnect_SRCS resources.qrc)

qt5_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

add_executable(kdeconnect-app
    main.cpp
    ${kdeconnect_SRCS}
    ${kdeconnect_custom_icons_SRCS}
)
target_link_libraries(kdeconnect-app Qt5::Quick Qt5::QuickControls2 Qt5::Widgets KF5::CoreAddons KF5::I18n)

install(TARGETS kdeconnect-app ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.app.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
