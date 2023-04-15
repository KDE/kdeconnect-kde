qt5_add_resources(kdeconnect_SRCS resources.qrc)

qt5_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KDE_CONNECT
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kdeconnect-version.h
)

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

ecm_find_qmlmodule(org.kde.kirigamiaddons.labs.mobileform 0.1)

add_executable(kdeconnect-app
    main.cpp
    ${kdeconnect_SRCS}
    ${kdeconnect_custom_icons_SRCS}
)
target_link_libraries(kdeconnect-app Qt::Quick Qt::QuickControls2 Qt::Widgets KF${QT_MAJOR_VERSION}::CoreAddons KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::KCMUtils)

install(TARGETS kdeconnect-app ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.app.desktop DESTINATION ${KDE_INSTALL_APPDIR})
