ki18n_wrap_ui(telhandler_SRCS dialog.ui)

add_executable(kdeconnect-handler kdeconnect-handler.cpp ${telhandler_SRCS})

target_link_libraries(kdeconnect-handler
    kdeconnectinterfaces
    kdeconnectcore
    kdeconnectversion
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM
)

install(TARGETS kdeconnect-handler ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.handler.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
