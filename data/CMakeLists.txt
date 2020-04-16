install (FILES "kdeconnect.contract" DESTINATION ${KDE_INSTALL_DATADIR}/share/contractor/)
install (FILES "kdeconnect-thunar.desktop" DESTINATION ${KDE_INSTALL_DATADIR}/share/Thunar/sendto/)
install(PROGRAMS org.kde.kdeconnect_open.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

if(NOT SAILFISHOS)
    install(FILES org.kde.kdeconnect.kcm.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
endif()
