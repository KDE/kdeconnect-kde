add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-settings")

qt5_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

add_executable(kdeconnect-settings
    main.cpp
    ${kdeconnect_custom_icons_SRCS}
)

target_link_libraries(kdeconnect-settings kdeconnectversion KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::KCMUtils KF${QT_MAJOR_VERSION}::DBusAddons KF${QT_MAJOR_VERSION}::WindowSystem)

install(TARGETS kdeconnect-settings ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdeconnect-settings.desktop DESTINATION ${KDE_INSTALL_APPDIR})
