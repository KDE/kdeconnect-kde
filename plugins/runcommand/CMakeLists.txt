kdeconnect_add_plugin(kdeconnect_runcommand SOURCES runcommandplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_runcommand
    HEADER plugin_runcommand_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_RUNCOMMAND CATEGORY_NAME kdeconnect.plugin.runcommand
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin runcommand)")
target_link_libraries(kdeconnect_runcommand
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::KCMUtils
    KF${QT_MAJOR_VERSION}::I18n)

#----------------------
kdeconnect_add_kcm(kdeconnect_runcommand_config SOURCES runcommand_config.cpp)
target_link_libraries(kdeconnect_runcommand_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigWidgets
)
