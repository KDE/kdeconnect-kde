find_package(KF5 REQUIRED COMPONENTS Notifications)

include_directories(${CMAKE_BINARY_DIR})

add_subdirectory(telepathy-cm)

qt5_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/telepathy-cm/protocol.h ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterface.xml)
qt5_add_dbus_interface(kdeconnect_telepathy_SRCS ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterface.xml kdeconnectinterface)

kdeconnect_add_plugin(kdeconnect_telepathy JSON kdeconnect_telepathy.json
    SOURCES telepathyplugin.cpp ${kdeconnect_telepathy_SRCS})

target_link_libraries(kdeconnect_telepathy
    kdeconnectcore
    KF5::I18n
    KF5::Notifications
    Qt5::DBus
)
