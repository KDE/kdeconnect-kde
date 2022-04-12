find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)

set(kdeconnect_libraries
    kdeconnectcore
    kdeconnectversion
    KF5::I18n
    Qt5::DBus
    Qt5::Network
    Qt5::Test
)

if(${KF5KIO_FOUND})
    list(APPEND kdeconnect_libraries KF5::KIOWidgets)
endif()

ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testsocketlinereader.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})
