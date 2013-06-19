
set(kcm_SRCS
    kcm.cpp
)

qt4_automoc(${kcm_SRCS})

kde4_add_ui_files(kcm_SRCS kcm.ui wizzard.ui)

kde4_add_plugin(kdeconnect-kcm ${kcm_SRCS})

target_link_libraries(kdeconnect-kcm
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBRARY}
    ${KDE4_KIO_LIBRARY}
)

install(TARGETS kdeconnect-kcm DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kdeconnect-kcm.desktop DESTINATION ${SERVICES_INSTALL_DIR})
