find_package(KF5 REQUIRED COMPONENTS Notifications)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../notifications/) # needed for the sendreplydialog

ki18n_wrap_ui(kdeconnect_telephony_SRCS ../notifications/sendreplydialog.ui)
kdeconnect_add_plugin(kdeconnect_telephony JSON kdeconnect_telephony.json SOURCES telephonyplugin.cpp ../notifications/sendreplydialog.cpp ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF5::I18n
    KF5::Notifications
    Qt5::DBus
    Qt5::Widgets
)
