find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)

set(kdeconnect_libraries
    kdeconnectcore
    kdeconnectversion
    KF${QT_MAJOR_VERSION}::I18n
    Qt::DBus
    Qt::Network
    Qt::Test
)

if(${KF5KIO_FOUND})
    list(APPEND kdeconnect_libraries KF${QT_MAJOR_VERSION}::KIOWidgets)
endif()

ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testsocketlinereader.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})
