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
endif()


kdeconnect_add_plugin(kdeconnect_screensaver_inhibit SOURCES ${kdeconnect_screensaver_inhibit_SRCS})

ecm_qt_declare_logging_category(kdeconnect_screensaver_inhibit
    HEADER kdeconnect_screensaverinhibit_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SCREENSAVERINHIBIT CATEGORY_NAME kdeconnect.plugin.screensaverinhibit
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin screensaverinhibit)")

target_link_libraries(kdeconnect_screensaver_inhibit kdeconnectcore kdeconnectinterfaces)
if(NOT APPLE AND NOT WIN32)
    target_link_libraries(kdeconnect_screensaver_inhibit
        Qt::DBus
        KF${QT_MAJOR_VERSION}::I18n
    )
endif()

