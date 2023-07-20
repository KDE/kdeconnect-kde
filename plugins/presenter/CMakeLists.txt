qt_add_resources(presenter_SRCS assets.qrc)

kdeconnect_add_plugin(kdeconnect_presenter SOURCES presenterplugin.cpp ${presenter_SRCS})
ecm_qt_declare_logging_category(kdeconnect_presenter
    HEADER plugin_presenter_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PRESENT CATEGORY_NAME kdeconnect.plugin.presenter
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin presenter)")

target_link_libraries(kdeconnect_presenter
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_MAJOR_VERSION}::Quick
    KF${QT_MAJOR_VERSION}::I18n
)
