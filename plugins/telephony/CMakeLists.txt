find_package(KF5 REQUIRED COMPONENTS Notifications)

include_directories(${CMAKE_BINARY_DIR})

set(kdeconnect_telephony_SRCS
    telephonyplugin.cpp
    sendsmsdialog.cpp
)

qt5_generate_dbus_interface(${CMAKE_SOURCE_DIR}/telepathy-cm/protocol.h ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterface.xml)
qt5_add_dbus_interface(kdeconnect_telephony_SRCS ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterface.xml kdeconnectinterface)

kdeconnect_add_plugin(kdeconnect_telephony JSON kdeconnect_telephony.json SOURCES ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF5::I18n
    KF5::Notifications
    Qt5::DBus
)
