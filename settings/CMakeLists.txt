add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-settings")

include_directories(${CMAKE_BINARY_DIR})

add_executable(kdeconnect-settings main.cpp)
target_link_libraries(kdeconnect-settings KF5::I18n KF5::KCMUtils KF5::DBusAddons)

install(TARGETS kdeconnect-settings ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdeconnect.settings.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
