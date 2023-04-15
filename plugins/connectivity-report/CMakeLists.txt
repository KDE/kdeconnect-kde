set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_connectivity_report_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_CONNECTIVITY_REPORT CATEGORY_NAME kdeconnect.plugin.connectivity_report
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin connectivity_report)")

set(kdeconnect_connectivity_report_SRCS
    connectivity_reportplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_connectivity_report SOURCES ${kdeconnect_connectivity_report_SRCS})

target_link_libraries(kdeconnect_connectivity_report
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::Solid
    KF${QT_MAJOR_VERSION}::I18n
)
