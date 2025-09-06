install (FILES "kdeconnect.contract" DESTINATION ${KDE_INSTALL_DATADIR}/contractor/)
install (FILES "kdeconnect-thunar.desktop" DESTINATION ${KDE_INSTALL_DATADIR}/Thunar/sendto/)
install (FILES "kdeconnect-dde.desktop" DESTINATION ${KDE_INSTALL_DATADIR}/deepin/dde-file-manager/oem-menuextensions/)

install(FILES org.kde.kdeconnect.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

if(INSTALL_UFW_APPLICATION_RULE)
    install(FILES kdeconnect.ufw DESTINATION "${KDE_INSTALL_FULL_SYSCONFDIR}/ufw/applications.d" RENAME kdeconnect)
endif()
