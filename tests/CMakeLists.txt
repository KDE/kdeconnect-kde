find_package(Qt5 REQUIRED COMPONENTS Test)
find_package(KF5 REQUIRED COMPONENTS KIO Notifications IconThemes)

include_directories(
    ${KDEConnectCore_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_libraries
    kdeconnectcore
    KF5::KIOWidgets
    Qt5::Network
    Qt5::Test
    qca-qt5
)

ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(networkpackagetests.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testsocketlinereader.cpp ../core/backends/lan/socketlinereader.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(downloadjobtest.cpp ../core/backends/lan/downloadjob.cpp TEST_NAME downloadjobtest LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testnotificationlistener.cpp
             ../plugins/sendnotifications/sendnotificationsplugin.cpp
             ../plugins/sendnotifications/notificationslistener.cpp
             ../plugins/sendnotifications/notifyingapplication.cpp
             TEST_NAME testnotificationlistener
             LINK_LIBRARIES ${kdeconnect_libraries} Qt5::DBus KF5::Notifications KF5::IconThemes)
