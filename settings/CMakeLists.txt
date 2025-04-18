add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-settings")

qt_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

add_executable(kdeconnect-settings
    main.cpp
    ${kdeconnect_custom_icons_SRCS}
)

target_link_libraries(kdeconnect-settings
    kdeconnectversion
    Qt::QuickControls2
    KF6::I18n
    KF6::KCMUtils
    KF6::DBusAddons
    KF6::WindowSystem
    KF6::Crash)

install(TARGETS kdeconnect-settings ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdeconnect-settings.desktop DESTINATION ${KDE_INSTALL_APPDIR})
