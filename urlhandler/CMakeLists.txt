add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-urlhandler")

ki18n_wrap_ui(telhandler_SRCS dialog.ui)

qt_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

add_executable(kdeconnect-handler kdeconnect-handler.cpp ${telhandler_SRCS} ${kdeconnect_custom_icons_SRCS})


target_link_libraries(kdeconnect-handler
    kdeconnectinterfaces
    kdeconnectcore
    kdeconnectversion
    Qt::Widgets
    KF6::CoreAddons
    KF6::ConfigWidgets
    KF6::DBusAddons
    KF6::I18n
    KF6::KIOCore KF6::KIOFileWidgets KF6::KIOWidgets
)

install(TARGETS kdeconnect-handler ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.handler.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
