# Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
# Redistribution and use is allowed according to the terms of the BSD license.


# Thoroughly inspired in kdevplatform_add_plugin
function(kdeconnect_add_plugin plugin)
    set(options )
    set(oneValueArgs JSON)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(KC_ADD_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_filename_component(json "${KC_ADD_PLUGIN_JSON}" REALPATH)

    # ensure we recompile the corresponding object files when the json file changes
    set(dependent_sources )
    foreach(source ${KC_ADD_PLUGIN_SOURCES})
        get_filename_component(source "${source}" REALPATH)
        if(EXISTS "${source}")
            file(STRINGS "${source}" match REGEX "K_PLUGIN_FACTORY_WITH_JSON")
            if(match)
                list(APPEND dependent_sources "${source}")
            endif()
        endif()
    endforeach()
    if(NOT dependent_sources)
        # fallback to all sources - better safe than sorry...
        set(dependent_sources ${KC_ADD_PLUGIN_SOURCES})
    endif()
    set_property(SOURCE ${dependent_sources} APPEND PROPERTY OBJECT_DEPENDS ${json})

    add_library(${plugin} MODULE ${KC_ADD_PLUGIN_SOURCES})
    set_property(TARGET ${plugin} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS ${json})

    install(TARGETS ${plugin} DESTINATION ${PLUGIN_INSTALL_DIR}/kdeconnect)
endfunction()
