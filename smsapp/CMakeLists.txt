qt5_add_resources(KCSMS_SRCS resources.qrc)

find_package(KF5People)


add_executable(kdeconnect-sms
    main.cpp
    conversationmodel.cpp
    conversationlistmodel.cpp
    ${KCSMS_SRCS})

target_link_libraries(kdeconnect-sms
    kdeconnectinterfaces
    Qt5::Quick
    Qt5::Widgets
    KF5::CoreAddons
    KF5::DBusAddons
    KF5::I18n
    KF5::People
    )

install(TARGETS kdeconnect-sms ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.sms.desktop DESTINATION ${KDE_INSTALL_APPDIR})
