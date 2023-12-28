add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kded")

add_executable(kdeconnectd kdeconnectd.cpp ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)
ecm_qt_declare_logging_category(kdeconnectd
    HEADER kdeconnectd_debug.h
    IDENTIFIER KDECONNECT_DAEMON CATEGORY_NAME kdeconnect.daemon
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (daemon)")
target_link_libraries(kdeconnectd
    kdeconnectcore kdeconnectversion
    KF6::KIOWidgets
    KF6::DBusAddons
    KF6::Notifications
    KF6::I18n
    KF6::WindowSystem
    Qt::Widgets)

ecm_mark_nongui_executable(kdeconnectd)

configure_file(org.kde.kdeconnect.daemon.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop)

# Conditionally configure the daemon service according to the target OS
if(WIN32)
    set(SERV_EXEC kdeconnectd)
else()
    set(SERV_EXEC ${KDE_INSTALL_FULL_LIBEXECDIR}/kdeconnectd)
endif()
configure_file(org.kde.kdeconnect.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.daemon.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
install(TARGETS kdeconnectd DESTINATION ${KDE_INSTALL_LIBEXECDIR})

