# SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
# Redistribution and use is allowed according to the terms of the BSD license.


function(kdeconnect_add_plugin)
    kcoreaddons_add_plugin(${ARGN} INSTALL_NAMESPACE kdeconnect)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}_config.qml")
        install(FILES "${ARGV0}_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
    endif()
endfunction()
