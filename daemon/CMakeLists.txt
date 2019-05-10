project(kdeconnectd)

add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kded")

add_executable(kdeconnectd kdeconnectd.cpp)
target_include_directories(kdeconnectd PUBLIC ${CMAKE_BINARY_DIR})
target_link_libraries(kdeconnectd kdeconnectcore KF5::KIOWidgets KF5::DBusAddons KF5::Notifications KF5::I18n Qt5::Widgets)

ecm_mark_nongui_executable(kdeconnectd)

configure_file(org.kde.kdeconnect.daemon.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop)
configure_file(org.kde.kdeconnect.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})

