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
    KF6::I18n
    KF6::KIOFileWidgets
    KF6::KIOWidgets
    KF6::Notifications
)
