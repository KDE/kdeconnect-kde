
add_subdirectory(declarativeplugin)

if (NOT SAILFISHOS)
install(DIRECTORY package/
        DESTINATION ${DATA_INSTALL_DIR}/plasma/plasmoids/org.kde.kdeconnect)

install(FILES package/metadata.desktop
        DESTINATION ${SERVICES_INSTALL_DIR}
        RENAME plasma-kdeconnect.desktop)
endif()
