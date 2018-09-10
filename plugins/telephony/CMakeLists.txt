set(kdeconnect_telephony_SRCS
    telephonyplugin.cpp
)

include_directories(${CMAKE_BINARY_DIR})

kdeconnect_add_plugin(kdeconnect_telephony JSON kdeconnect_telephony.json SOURCES ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF5::I18n
    KF5::Notifications
    Qt5::DBus
)
