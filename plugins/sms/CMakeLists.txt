set(kdeconnect_sms_SRCS
    smsplugin.cpp
    conversationsdbusinterface.cpp
)

include_directories(${CMAKE_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../notifications/) # needed for the sendreplydialog

ki18n_wrap_ui(kdeconnect_sms_SRCS ../notifications/sendreplydialog.ui)

kdeconnect_add_plugin(kdeconnect_sms JSON kdeconnect_sms.json SOURCES ../notifications/sendreplydialog.cpp ${kdeconnect_sms_SRCS})


target_link_libraries(kdeconnect_sms
    kdeconnectcore
    kdeconnectinterfaces
    Qt5::DBus
    KF5::I18n
    KF5::Notifications
    Qt5::Widgets
)
