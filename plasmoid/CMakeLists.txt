kpackage_install_package(package org.kde.kdeconnect plasmoids plasma NO_DEPRECATED_WARNING)

if("${QT_MAJOR_VERSION}" STREQUAL 5)
install(FILES package/metadata.desktop
        DESTINATION ${KDE_INSTALL_KSERVICESDIR}
        RENAME plasma-kdeconnect.desktop)
endif()
