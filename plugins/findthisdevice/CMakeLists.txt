# Target kdeconnect_findthisdevice_qmlhelper

ecm_add_qml_module(kdeconnect_findthisdevice_qmlhelper
    URI "org.kde.kdeconnect.private.findthisdevice"
    GENERATE_PLUGIN_SOURCE
)

target_sources(kdeconnect_findthisdevice_qmlhelper PRIVATE findthisdevicehelper.cpp)

target_link_libraries(kdeconnect_findthisdevice_qmlhelper PRIVATE
    Qt::Qml
)

ecm_finalize_qml_module(kdeconnect_findthisdevice_qmlhelper DESTINATION ${KDE_INSTALL_QMLDIR})

# Target kdeconnect_findthisdevice

kdeconnect_add_plugin(kdeconnect_findthisdevice SOURCES findthisdeviceplugin.cpp findthisdevicehelper.cpp)

target_link_libraries(kdeconnect_findthisdevice
    kdeconnectcore
    Qt::Core
    Qt::Multimedia
    Qt::DBus
    Qt::Qml
)

if (NOT WIN32)
    target_link_libraries(kdeconnect_findthisdevice
        KF6::PulseAudioQt
    )
endif()

# Target kdeconnect_findthisdevice_config

kdeconnect_add_kcm(kdeconnect_findthisdevice_config SOURCES findthisdevice_config.cpp findthisdevicehelper.cpp)

ki18n_wrap_ui(kdeconnect_findthisdevice_config findthisdevice_config.ui)
target_link_libraries(kdeconnect_findthisdevice_config
    kdeconnectpluginkcm
    Qt::Multimedia
    Qt::Qml
    KF6::I18n
    KF6::CoreAddons
    KF6::ConfigWidgets
    KF6::KIOWidgets # KUrlRequester
)
