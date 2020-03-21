find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)

include_directories(
    ${KDEConnectCore_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/../plugins/sendnotifications/
    ${CMAKE_CURRENT_BINARY_DIR}/../smsapp/
)

set(kdeconnect_libraries
	kdeconnectcore
    KF5::I18n
    Qt5::DBus
    Qt5::Network
    Qt5::Test
    ${Qca_LIBRARY}
)

if(${KF5KIO_FOUND})
    list(APPEND kdeconnect_libraries KF5::KIOWidgets)
endif()

set(kdeconnect_sms_libraries
    kdeconnectsmshelper
    Qt5::Test
)

ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testsocketlinereader.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})

if(PRIVATE_DBUS_ENABLED)
    ecm_add_test(testprivatedbus.cpp LINK_LIBRARIES ${kdeconnect_libraries})
endif()
