qt5_add_resources(kdeconnect_SRCS resources.qrc)

qt5_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

if (WIN32)
    include(ECMAddAppIcon)
    ecm_add_app_icon(kdeconnect_SRCS ICONS
        ${CMAKE_SOURCE_DIR}/icons/app/sc-apps-kdeconnect.svg
        ${CMAKE_SOURCE_DIR}/icons/app/16-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/app/24-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/app/32-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/app/48-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/app/64-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/app/128-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/app/256-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/app/1024-apps-kdeconnect.png
    )
endif()

add_executable(kdeconnect-app
    main.cpp
    ${kdeconnect_SRCS}
    ${kdeconnect_custom_icons_SRCS}
)
target_link_libraries(kdeconnect-app Qt5::Quick Qt5::QuickControls2 Qt5::Widgets KF5::CoreAddons KF5::I18n KF5::KCMUtils)

install(TARGETS kdeconnect-app ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.app.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
