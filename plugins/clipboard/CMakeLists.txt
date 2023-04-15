
set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_clipboard_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_CLIPBOARD CATEGORY_NAME kdeconnect.plugin.clipboard
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin clipboard)")

set(kdeconnect_clipboard_SRCS
    clipboardplugin.cpp
    clipboardlistener.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_clipboard SOURCES ${kdeconnect_clipboard_SRCS})

target_link_libraries(kdeconnect_clipboard kdeconnectcore
                                    KF${QT_MAJOR_VERSION}::GuiAddons
                                    ${kdeconnect_clipboard_WL_LINK_LIBS}
                                    )

kcoreaddons_add_plugin(kdeconnect_clipboard_config INSTALL_NAMESPACE "kdeconnect/kcms")

ki18n_wrap_ui(kdeconnect_clipboard_config clipboard_config.ui)
target_sources(kdeconnect_clipboard_config PRIVATE clipboard_config.cpp ${debug_file_SRCS})
target_link_libraries( kdeconnect_clipboard_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KCMUtils
)
