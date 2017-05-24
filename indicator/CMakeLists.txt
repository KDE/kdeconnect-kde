find_package(KF5Notifications REQUIRED)
find_package(KF5KCMUtils REQUIRED)

set(indicator_SRCS
    main.cpp
    deviceindicator.cpp
)

include(ECMAddAppIcon)
ecm_add_app_icon(indicator_SRCS ICONS
    ../icon/16-apps-kdeconnect.png
    ../icon/22-apps-kdeconnect.png
    ../icon/32-apps-kdeconnect.png
    ../icon/48-apps-kdeconnect.png
    ../icon/64-apps-kdeconnect.png
    ../icon/128-apps-kdeconnect.png
    ../icon/256-apps-kdeconnect.png
)

add_executable(kdeconnect-indicator ${indicator_SRCS})
target_link_libraries(kdeconnect-indicator Qt5::Widgets KF5::CoreAddons KF5::I18n KF5::Notifications KF5::DBusAddons KF5::KCMUtils kdeconnectinterfaces)

install(TARGETS kdeconnect-indicator ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.nonplasma.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
