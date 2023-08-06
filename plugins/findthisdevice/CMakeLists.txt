kdeconnect_add_plugin(kdeconnect_findthisdevice SOURCES findthisdeviceplugin.cpp)

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

kdeconnect_add_kcm(kdeconnect_findthisdevice_config SOURCES findthisdevice_config.cpp)

ki18n_wrap_ui(kdeconnect_findthisdevice_config findthisdevice_config.ui)
target_link_libraries(kdeconnect_findthisdevice_config
    kdeconnectpluginkcm
    Qt::Multimedia
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigWidgets
    KF${QT_MAJOR_VERSION}::KIOWidgets # KUrlRequester
)
