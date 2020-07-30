add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kded")

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER kdeconnectd_debug.h
    IDENTIFIER KDECONNECT_DAEMON CATEGORY_NAME kdeconnect.daemon
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (daemon)")

add_executable(kdeconnectd kdeconnectd.cpp ${debug_file_SRCS})
target_link_libraries(kdeconnectd kdeconnectcore kdeconnectversion KF5::KIOWidgets KF5::DBusAddons KF5::Notifications KF5::I18n Qt5::Widgets)

ecm_mark_nongui_executable(kdeconnectd)

configure_file(org.kde.kdeconnect.daemon.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop)
configure_file(org.kde.kdeconnect.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})

