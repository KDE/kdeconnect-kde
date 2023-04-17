# SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
# Redistribution and use is allowed according to the terms of the BSD license.


if (SAILFISHOS)
    function(kdeconnect_add_plugin plugin)
        set(options)
        set(oneValueArgs JSON)
        set(multiValueArgs SOURCES)
        cmake_parse_arguments(KC_ADD_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        if(NOT KC_ADD_PLUGIN_SOURCES)
            message(FATAL_ERROR "kdeconnect_add_plugin called without SOURCES parameter")
        endif()
        get_filename_component(json "${KC_ADD_PLUGIN_JSON}" REALPATH)

        add_library(${plugin} STATIC ${KC_ADD_PLUGIN_SOURCES})
        set_property(TARGET ${plugin} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS ${json})
        set_property(TARGET ${plugin} APPEND PROPERTY COMPILE_DEFINITIONS QT_STATICPLUGIN)
    endfunction()
else()
    function(kdeconnect_add_plugin)
        kcoreaddons_add_plugin(${ARGN} INSTALL_NAMESPACE kdeconnect)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV0}_config.qml")
            install(FILES "${ARGV0}_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
        endif()
    endfunction()
endif()
