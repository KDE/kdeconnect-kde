add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-settings")

add_executable(kdeconnect-settings main.cpp)
target_link_libraries(kdeconnect-settings kdeconnectversion KF5::I18n KF5::KCMUtils KF5::DBusAddons)

install(TARGETS kdeconnect-settings ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdeconnect.settings.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
