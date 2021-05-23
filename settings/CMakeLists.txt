add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-settings")

qt5_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

add_executable(kdeconnect-settings
    main.cpp
    ${kdeconnect_custom_icons_SRCS}
)

target_link_libraries(kdeconnect-settings kdeconnectversion KF5::I18n KF5::KCMUtils KF5::DBusAddons)

install(TARGETS kdeconnect-settings ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdeconnect.settings.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
