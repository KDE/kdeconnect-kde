find_package(Qt5 REQUIRED COMPONENTS Test)
find_package(KF5 REQUIRED COMPONENTS KIO Notifications IconThemes)

include_directories(
    ${KDEConnectCore_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_libraries
    kdeconnectcore
    KF5::I18n
    KF5::KIOWidgets
    Qt5::DBus
    Qt5::Network
    Qt5::Test
    qca-qt5
)

ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(networkpackagetests.cpp LINK_LIBRARIES ${kdeconnect_libraries})

ecm_add_test(testsocketlinereader.cpp ../core/backends/lan/socketlinereader.cpp ../core/backends/lan/server.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})

set(testsslsocketlinereader_sources
    ../core/backends/lan/server.cpp
    ../core/backends/lan/socketlinereader.cpp
)
ecm_add_test(testsslsocketlinereader.cpp ${testsslsocketlinereader_sources} TEST_NAME testsslsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})

set(kdeconnectconfigtest_sources
    ../core/backends/devicelink.cpp
    ../core/backends/pairinghandler.cpp
    ../core/dbushelper.cpp
    ../core/device.cpp
    ../core/pluginloader.cpp
)
ecm_add_test(kdeconnectconfigtest.cpp ${kdeconnectconfigtest_sources} TEST_NAME kdeconnectconfigtest LINK_LIBRARIES ${kdeconnect_libraries})

set(lanlinkprovidertest_sources
    ../core/backends/devicelink.cpp
    ../core/backends/lan/downloadjob.cpp
    ../core/backends/lan/landevicelink.cpp
    ../core/backends/lan/lanlinkprovider.cpp
    ../core/backends/lan/lanpairinghandler.cpp
    ../core/backends/lan/server.cpp
    ../core/backends/lan/socketlinereader.cpp
    ../core/backends/lan/uploadjob.cpp
    ../core/backends/linkprovider.cpp
    ../core/backends/pairinghandler.cpp
    ../core/device.cpp
    ../core/pluginloader.cpp
)
ecm_add_test(lanlinkprovidertest.cpp  ${lanlinkprovidertest_sources}  TEST_NAME lanlinkprovidertest LINK_LIBRARIES ${kdeconnect_libraries})

ecm_add_test(devicetest.cpp ${lanlinkprovidertest_sources} TEST_NAME devicetest LINK_LIBRARIES ${kdeconnect_libraries})

ecm_add_test(downloadjobtest.cpp ../core/backends/lan/downloadjob.cpp TEST_NAME downloadjobtest LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testnotificationlistener.cpp
             ../plugins/sendnotifications/sendnotificationsplugin.cpp
             ../plugins/sendnotifications/notificationslistener.cpp
             ../plugins/sendnotifications/notifyingapplication.cpp
             TEST_NAME testnotificationlistener
             LINK_LIBRARIES ${kdeconnect_libraries} Qt5::DBus KF5::Notifications KF5::IconThemes)
