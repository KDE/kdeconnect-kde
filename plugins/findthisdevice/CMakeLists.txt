set(kdeconnect_findthisdevice_SRCS
    findthisdeviceplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_findthisdevice
    JSON kdeconnect_findthisdevice.json
    SOURCES ${kdeconnect_findthisdevice_SRCS})

target_link_libraries(kdeconnect_findthisdevice
    kdeconnectcore
    Qt5::Core
    Qt5::Multimedia
    Qt5::DBus
)

if (NOT WIN32)
    target_link_libraries(kdeconnect_findthisdevice
        KF5::PulseAudioQt
    )
endif()

set(kdeconnect_findthisdevice_config_SRCS findthisdevice_config.cpp)
ki18n_wrap_ui(kdeconnect_findthisdevice_config_SRCS findthisdevice_config.ui)

add_library(kdeconnect_findthisdevice_config MODULE ${kdeconnect_findthisdevice_config_SRCS})
target_link_libraries(kdeconnect_findthisdevice_config
    kdeconnectpluginkcm
    Qt5::Multimedia
    KF5::I18n
    KF5::CoreAddons
    KF5::ConfigWidgets
    KF5::KIOWidgets     # KUrlRequester
)

install(TARGETS kdeconnect_findthisdevice_config DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES kdeconnect_findthisdevice_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
