if(APPLE)
    set(kdeconnect_screensaver_inhibit_SRCS
        screensaverinhibitplugin-macos.cpp
    )
elseif(WIN32)
    set(kdeconnect_screensaver_inhibit_SRCS
        screensaverinhibitplugin-win.cpp
    )
else()
    set(kdeconnect_screensaver_inhibit_SRCS
        screensaverinhibitplugin.cpp
    )
    qt_add_dbus_interface(kdeconnect_screensaver_inhibit_SRCS org.freedesktop.ScreenSaver.xml screensaverinterface)
endif()

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER kdeconnect_screensaverinhibit_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SCREENSAVERINHIBIT CATEGORY_NAME kdeconnect.plugin.screensaverinhibit
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin screensaverinhibit)")

kdeconnect_add_plugin(kdeconnect_screensaver_inhibit SOURCES ${kdeconnect_screensaver_inhibit_SRCS} ${debug_file_SRCS})

target_link_libraries(kdeconnect_screensaver_inhibit kdeconnectcore)
if(NOT APPLE AND NOT WIN32)
    target_link_libraries(kdeconnect_screensaver_inhibit
        Qt::DBus
        KF${QT_MAJOR_VERSION}::I18n
    )
endif()

