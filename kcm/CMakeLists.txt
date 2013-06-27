
set(kcm_SRCS devicesmodel.cpp
    kcm.cpp
    wizard.cpp
    daemondbusinterface.cpp
)

qt4_add_dbus_interface(
   kcm_SRCS
   ${CMAKE_BINARY_DIR}/daemon/org.kde.kdeconnect.xml
   org_kde_kdeconnect_interface
)

kde4_add_ui_files(kcm_SRCS kcm.ui wizard.ui)

kde4_add_plugin(kdeconnect-kcm ${kcm_SRCS})

target_link_libraries(kdeconnect-kcm
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBRARY}
    ${KDE4_KIO_LIBRARY}
)

install(TARGETS kdeconnect-kcm DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kdeconnect-kcm.desktop DESTINATION ${SERVICES_INSTALL_DIR})
