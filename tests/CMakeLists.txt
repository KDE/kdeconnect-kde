find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)

set(kdeconnect_libraries
    kdeconnectcore
    kdeconnectinterfaces
    kdeconnectsmshelper
    kdeconnectversion
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KIOWidgets
    Qt::DBus
    Qt::Network
    KF${QT_MAJOR_VERSION}::People
    Qt::Qml
    Qt::Test
)

ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(smshelpertest.cpp LINK_LIBRARIES ${kdeconnect_libraries})

if(MDNS_ENABLED)
    ecm_add_test(mdnstest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
endif()
