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

kdeconnect_add_plugin(kdeconnect_connectivity_report JSON kdeconnect_connectivity_report.json SOURCES ${kdeconnect_connectivity_report_SRCS})

target_link_libraries(kdeconnect_connectivity_report
    kdeconnectcore
    Qt5::DBus
    KF5::Solid
    KF5::I18n
)
