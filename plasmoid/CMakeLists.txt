kpackage_install_package(package org.kde.kdeconnect plasmoids plasma NO_DEPRECATED_WARNING)

install(FILES package/metadata.desktop
        DESTINATION ${SERVICES_INSTALL_DIR}
        RENAME plasma-kdeconnect.desktop)
