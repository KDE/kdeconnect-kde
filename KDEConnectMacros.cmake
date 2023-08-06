# SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
# Redistribution and use is allowed according to the terms of the BSD license.


function(kdeconnect_add_plugin)
    kcoreaddons_add_plugin(${ARGN} INSTALL_NAMESPACE kdeconnect)
endfunction()

function(kdeconnect_add_kcm plugin)
    kcoreaddons_add_plugin(${plugin} ${ARGN} INSTALL_NAMESPACE kdeconnect/kcms)
    install(FILES "${plugin}.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
endfunction()
