
set(kcm_SRCS devicesmodel.cpp
    kcm.cpp
    #wizard.cpp
    dbusinterfaces.cpp
)

qt4_add_dbus_interface(
   kcm_SRCS
   ${CMAKE_BINARY_DIR}/daemon/org.kde.kdeconnect.daemon.xml
   daemoninterface
)

qt4_add_dbus_interface(
   kcm_SRCS
   ${CMAKE_BINARY_DIR}/daemon/org.kde.kdeconnect.device.xml
   deviceinterface
)

kde4_add_ui_files(kcm_SRCS kcm.ui wizard.ui)

kde4_add_plugin(kcm_kdeconnect ${kcm_SRCS})

target_link_libraries(kcm_kdeconnect
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBRARY}
    ${KDE4_KCMUTILS_LIBS}
)

add_dependencies(kcm_kdeconnect
    org.kde.kdeconnect.daemon.xml
    org.kde.kdeconnect.device.xml
)

install(TARGETS kcm_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kcm_kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR})
