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

kdeconnect_add_plugin(kdeconnect_sftp SOURCES ${kdeconnect_sftp_SRCS})
target_link_libraries(kdeconnect_sftp
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KIOFileWidgets
    KF${QT_MAJOR_VERSION}::KIOWidgets
    KF${QT_MAJOR_VERSION}::Notifications
)
