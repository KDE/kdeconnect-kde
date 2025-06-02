find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)

set(kdeconnect_libraries
    kdeconnectcore
    kdeconnectinterfaces
    kdeconnectsmshelper
    kdeconnectversion
    KF6::GuiAddons
    KF6::I18n
    KF6::KIOWidgets
    KF6::Notifications
    Qt::DBus
    Qt::Network
    KF6::People
    Qt::Qml
    Qt::Test
)

ecm_add_test(notificationstest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(smshelpertest.cpp LINK_LIBRARIES ${kdeconnect_libraries})

if(MDNS_ENABLED)
    ecm_add_test(mdnstest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
endif()
