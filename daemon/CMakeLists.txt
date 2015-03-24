project(kdeconnectd)

find_package(KF5 REQUIRED COMPONENTS Notifications)

add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-daemon")

add_executable(kdeconnectd kdeconnectd.cpp)
target_link_libraries(kdeconnectd kdeconnectcore KF5::DBusAddons KF5::Notifications KF5::I18n Qt5::Widgets)

configure_file(kdeconnectd.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectd.desktop)
configure_file(org.kde.kdeconnect.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectd.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})
