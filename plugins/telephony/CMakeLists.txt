set(kdeconnect_telephony_SRCS
    telephonyplugin.cpp
    conversationsdbusinterface.cpp
)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../notifications/) # needed for the sendreplydialog

ki18n_wrap_ui(kdeconnect_telephony_SRCS ../notifications/sendreplydialog.ui)
kdeconnect_add_plugin(kdeconnect_telephony JSON kdeconnect_telephony.json SOURCES ../notifications/sendreplydialog.cpp ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    kdeconnectinterfaces
    KF5::I18n
    KF5::Notifications
    Qt5::DBus
    Qt5::Widgets
)
