qt5_add_resources(KCSMS_SRCS resources.qrc)

add_executable(kdeconnect-sms main.cpp ${KCSMS_SRCS})
target_link_libraries(kdeconnect-sms Qt5::Quick Qt5::Widgets KF5::DBusAddons KF5::CoreAddons KF5::I18n)

install(TARGETS kdeconnect-sms ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.sms.desktop DESTINATION ${KDE_INSTALL_APPDIR})
