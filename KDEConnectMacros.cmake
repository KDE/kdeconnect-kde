# SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
# Redistribution and use is allowed according to the terms of the BSD license.

# Function to create the plugin and generate a logging category for it
function(kdeconnect_add_plugin plugin_name)
    kcoreaddons_add_plugin(${plugin_name} ${ARGN} INSTALL_NAMESPACE kdeconnect)

    string(REPLACE "kdeconnect_" "" PlUGIN_WITHOUT_PREFIX "${plugin_name}") # For the file name and description, we don't want this
    string(TOUPPER "${PlUGIN_WITHOUT_PREFIX}" PLUGIN_UPPER) # The identifier is all capy
    ecm_qt_declare_logging_category(${plugin_name}
        HEADER plugin_${PlUGIN_WITHOUT_PREFIX}_debug.h
        IDENTIFIER KDECONNECT_PLUGIN_${PLUGIN_UPPER} CATEGORY_NAME kdeconnect.plugin.${PlUGIN_WITHOUT_PREFIX}
        DEFAULT_SEVERITY Warning
        EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin ${PlUGIN_WITHOUT_PREFIX})")
endfunction()

