kdeconnect_add_plugin(kdeconnect_clipboard SOURCES clipboardplugin.cpp clipboardlistener.cpp)

target_link_libraries(kdeconnect_clipboard kdeconnectcore
    Qt::DBus
    KF6::GuiAddons
    ${kdeconnect_clipboard_WL_LINK_LIBS}
)

kdeconnect_add_kcm(kdeconnect_clipboard_config SOURCES clipboard_config.cpp)
ki18n_wrap_ui(kdeconnect_clipboard_config clipboard_config.ui)
target_link_libraries(kdeconnect_clipboard_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF6::I18n
    KF6::KCMUtils
)
