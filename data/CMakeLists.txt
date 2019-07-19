install (FILES "kdeconnect.contract" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/contractor/)
install (FILES "kdeconnect-thunar.desktop" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/Thunar/sendto/)
install(PROGRAMS org.kde.kdeconnect_open.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

if(NOT SAILFISHOS)
    install(FILES org.kde.kdeconnect.kcm.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
endif()
