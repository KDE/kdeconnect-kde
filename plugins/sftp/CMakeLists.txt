# Find fusermount -- otherwise fallback to umount
find_program(HAVE_FUSERMOUNT fusermount)
configure_file(config-sftp.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-sftp.h )

if(WIN32)
    set(kdeconnect_sftp_SRCS
        sftpplugin-win.cpp
    )
else()
    set(kdeconnect_sftp_SRCS
        mounter.cpp
        mountloop.cpp
        sftpplugin.cpp
    )
endif()

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_sftp_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SFTP CATEGORY_NAME kdeconnect.plugin.sftp
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin sftp)")

kdeconnect_add_plugin(kdeconnect_sftp SOURCES ${kdeconnect_sftp_SRCS} ${debug_file_SRCS})

target_link_libraries(kdeconnect_sftp
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KIOFileWidgets
    KF${QT_MAJOR_VERSION}::KIOWidgets
    KF${QT_MAJOR_VERSION}::Notifications
)
