qt_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KDECONNECT
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kdeconnect-version.h
)

if (WIN32)
    include(ECMAddAppIcon)
    ecm_add_app_icon(kdeconnect_SRCS ICONS
        ${CMAKE_SOURCE_DIR}/icons/app/sc-apps-kdeconnect.svg
        ${CMAKE_SOURCE_DIR}/icons/windows/32-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/windows/48-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/windows/64-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/windows/128-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/windows/256-apps-kdeconnect.png
        ${CMAKE_SOURCE_DIR}/icons/windows/512-apps-kdeconnect.png
    )
endif()

add_executable(kdeconnect-app
    main.cpp
    ${kdeconnect_SRCS}
    ${kdeconnect_custom_icons_SRCS}
)

ecm_add_qml_module(kdeconnect-app URI org.kde.kdeconnect.app)

ecm_target_qml_sources(kdeconnect-app SOURCES
    qml/Main.qml
    qml/mpris.qml
    qml/mousepad.qml
    qml/presentationRemote.qml
    qml/PluginItem.qml
    qml/DevicePage.qml
    qml/runcommand.qml
    qml/volume.qml
    qml/MprisSlider.qml
    qml/PluginSettings.qml
    qml/Settings.qml
    qml/WelcomePage.qml
)

target_link_libraries(kdeconnect-app PRIVATE Qt::Quick Qt::QuickControls2 Qt::Widgets KF6::CoreAddons KF6::I18n KF6::KCMUtils KF6::Crash KF6::ItemModels)

install(TARGETS kdeconnect-app ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.app.desktop DESTINATION ${KDE_INSTALL_APPDIR})
