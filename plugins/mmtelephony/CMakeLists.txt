kdeconnect_add_plugin(kdeconnect_mmtelephony SOURCES mmtelephonyplugin.cpp)
ecm_qt_declare_logging_category(kdeconnect_mmtelephony
    HEADER plugin_mmtelephony_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_MMTELEPHONY CATEGORY_NAME kdeconnect.plugin.mmtelephony
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin mmtelephony)")


target_link_libraries(kdeconnect_mmtelephony
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::ModemManagerQt
    KF${QT_MAJOR_VERSION}::I18n
)
