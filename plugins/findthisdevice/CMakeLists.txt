set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_findthisdevice_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_FINDTHISDEVICE CATEGORY_NAME kdeconnect.plugin.findthisdevice
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin findthisdevice)")

set(kdeconnect_findthisdevice_SRCS
    findthisdeviceplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_findthisdevice
    SOURCES ${kdeconnect_findthisdevice_SRCS})

target_link_libraries(kdeconnect_findthisdevice
    kdeconnectcore
    Qt::Core
    Qt::Multimedia
    Qt::DBus
)

if (NOT WIN32)
    target_link_libraries(kdeconnect_findthisdevice
        KF${QT_MAJOR_VERSION}::PulseAudioQt
    )
endif()

kcoreaddons_add_plugin(kdeconnect_findthisdevice_config INSTALL_NAMESPACE "kdeconnect/kcms")

ki18n_wrap_ui(kdeconnect_findthisdevice_config findthisdevice_config.ui)
target_sources(kdeconnect_findthisdevice_config PRIVATE findthisdevice_config.cpp ${debug_file_SRCS})
target_link_libraries(kdeconnect_findthisdevice_config
    kdeconnectpluginkcm
    Qt::Multimedia
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigWidgets
    KF${QT_MAJOR_VERSION}::KIOWidgets     # KUrlRequester
)
