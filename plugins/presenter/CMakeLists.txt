qt5_add_resources(presenter_SRCS assets.qrc)

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_presenter_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PRESENT CATEGORY_NAME kdeconnect.plugin.presenter
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin presenter)")

kdeconnect_add_plugin(kdeconnect_presenter SOURCES presenterplugin.cpp ${presenter_SRCS} ${debug_file_SRCS})
target_link_libraries(kdeconnect_presenter
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_MAJOR_VERSION}::Quick
    KF${QT_MAJOR_VERSION}::I18n
)
