set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_runcommand_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_RUNCOMMAND CATEGORY_NAME kdeconnect.plugin.runcommand
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin runcommand)")

set(kdeconnect_runcommand_SRCS
    runcommandplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_runcommand JSON kdeconnect_runcommand.json SOURCES ${kdeconnect_runcommand_SRCS})

target_link_libraries(kdeconnect_runcommand
    kdeconnectcore
    Qt5::DBus
    KF5::KCMUtils
    KF5::I18n)

#----------------------
if(NOT SAILFISHOS)
    set( kdeconnect_runcommand_config_SRCS runcommand_config.cpp )

    add_library(kdeconnect_runcommand_config MODULE ${kdeconnect_runcommand_config_SRCS} )
    target_link_libraries( kdeconnect_runcommand_config
        kdeconnectcore
        kdeconnectpluginkcm
        KF5::I18n
        KF5::CoreAddons
        KF5::ConfigWidgets

    )

    install(TARGETS kdeconnect_runcommand_config DESTINATION ${PLUGIN_INSTALL_DIR} )
    install(FILES kdeconnect_runcommand_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif()
