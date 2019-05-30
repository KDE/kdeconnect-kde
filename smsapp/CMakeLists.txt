qt5_add_resources(KCSMS_SRCS resources.qrc)

find_package(KF5People)

add_library(kdeconnectsms
    conversationmodel.cpp
    conversationlistmodel.cpp
)

set_target_properties(kdeconnectsms PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectsms EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectsms_export.h BASE_NAME KDEConnectSmsAppLib)
target_include_directories(kdeconnectsms PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# If ever this library is actually used by someone else, we should export these headers
target_link_libraries(kdeconnectsms
LINK_PRIVATE
    Qt5::Quick
    Qt5::Widgets
    KF5::People
    kdeconnectinterfaces
)

set(libkdeconnectsms_HEADERS
    conversationmodel.h
    conversationlistmodel.h
    ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectsms_export.h
)

add_executable(kdeconnect-sms
    main.cpp
    ${KCSMS_SRCS})

target_include_directories(kdeconnect-sms PUBLIC ${CMAKE_BINARY_DIR})

target_link_libraries(kdeconnect-sms
    kdeconnectsms
    kdeconnectinterfaces
    Qt5::Quick
    Qt5::Widgets
    KF5::CoreAddons
    KF5::DBusAddons
    KF5::I18n
    KF5::People
    )

install(TARGETS kdeconnect-sms ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeconnectsms ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install(PROGRAMS org.kde.kdeconnect.sms.desktop DESTINATION ${KDE_INSTALL_APPDIR})
