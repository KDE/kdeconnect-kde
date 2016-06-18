find_package(TelepathyQt5 0.9.5)
find_package(TelepathyQt5Service 0.9.7)

set_package_properties(TelepathyQt5 PROPERTIES
    PURPOSE "Allows integration with Telepathy. We suggest 'plugins/telepathy' to be packaged separately."
    TYPE OPTIONAL
)
set_package_properties(TelepathyQt5Service PROPERTIES
    PURPOSE "Allows integration with Telepathy. We suggest 'plugins/telepathy' to be packaged separately."
    TYPE OPTIONAL
)
if (NOT TelepathyQt5Service_FOUND OR NOT TelepathyQt5_FOUND)
    return()
endif()

include_directories(${CMAKE_BINARY_DIR})

add_subdirectory(telepathy-cm)

qt5_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/telepathy-cm/protocol.h ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterface.xml)
qt5_add_dbus_interface(kdeconnect_telepathy_SRCS ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterface.xml kdeconnectinterface)

kdeconnect_add_plugin(kdeconnect_telepathy JSON kdeconnect_telepathy.json
    SOURCES telepathyplugin.cpp ${kdeconnect_telepathy_SRCS})

target_link_libraries(kdeconnect_telepathy
    kdeconnectcore
    KF5::I18n
    Qt5::DBus
    Qt5::Gui
)
