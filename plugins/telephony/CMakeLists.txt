find_package(KF5 REQUIRED COMPONENTS Notifications)

include_directories(${CMAKE_BINARY_DIR})

kdeconnect_add_plugin(kdeconnect_telephony JSON kdeconnect_telephony.json SOURCES telephonyplugin.cpp sendsmsdialog.cpp)

target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF5::I18n
    KF5::Notifications
    Qt5::DBus
)
