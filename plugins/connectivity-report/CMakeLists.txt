kdeconnect_add_plugin(kdeconnect_connectivity_report SOURCES connectivity_reportplugin.cpp)
ecm_qt_declare_logging_category(kdeconnect_connectivity_report
    HEADER plugin_connectivity_report_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_CONNECTIVITY_REPORT CATEGORY_NAME kdeconnect.plugin.connectivity_report
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin connectivity_report)")

target_link_libraries(kdeconnect_connectivity_report
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Solid
    KF${QT_MAJOR_VERSION}::I18n
)
