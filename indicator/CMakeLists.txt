add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-indicator\")

qt_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

set(indicator_SRCS
    main.cpp
    deviceindicator.cpp
    ${kdeconnect_custom_icons_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/systray_actions/battery_action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/systray_actions/connectivity_action.cpp
)

include(ECMAddAppIcon)
ecm_add_app_icon(indicator_SRCS ICONS
    ${CMAKE_SOURCE_DIR}/icons/app/sc-apps-kdeconnect.svg
    ${CMAKE_SOURCE_DIR}/icons/windows/32-apps-kdeconnect.png
    ${CMAKE_SOURCE_DIR}/icons/windows/48-apps-kdeconnect.png
    ${CMAKE_SOURCE_DIR}/icons/windows/64-apps-kdeconnect.png
    ${CMAKE_SOURCE_DIR}/icons/windows/128-apps-kdeconnect.png
    ${CMAKE_SOURCE_DIR}/icons/windows/256-apps-kdeconnect.png
    ${CMAKE_SOURCE_DIR}/icons/windows/512-apps-kdeconnect.png
)

if (WIN32)
    list(APPEND indicator_SRCS indicatorhelper_win.cpp)
elseif (APPLE)
    list(APPEND indicator_SRCS indicatorhelper_mac.cpp serviceregister_mac.mm)
else ()
    list(APPEND indicator_SRCS indicatorhelper.cpp)
endif()

add_executable(kdeconnect-indicator ${indicator_SRCS})

ecm_qt_declare_logging_category(kdeconnect-indicator
    HEADER indicator_debug.h
    IDENTIFIER KDECONNECT_INDICATOR CATEGORY_NAME kdeconnect.indicator
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (indicator)")


target_include_directories(kdeconnect-indicator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/systray_actions)

target_link_libraries(kdeconnect-indicator Qt::Widgets Qt::QuickControls2 KF6::CoreAddons KF6::I18n KF6::Notifications KF6::DBusAddons KF6::ColorScheme KF6::Crash kdeconnectdbusinterfaces kdeconnectmodels kdeconnectcore kdeconnectversion)

if (WIN32)
    target_link_libraries(kdeconnect-indicator windowsapp)
else()
    target_link_libraries(kdeconnect-indicator KF6::StatusNotifierItem)
endif()

if (APPLE)
# Apple frameworks
target_link_libraries(kdeconnect-indicator "-framework AppKit")
# Apple app package
set_target_properties(kdeconnect-indicator PROPERTIES
    OUTPUT_NAME "KDE Connect"
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Connect"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${KDECONNECT_VERSION_STRING}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${KDECONNECT_VERSION_MAJOR}.${KDECONNECT_VERSION_MINOR}.${KDECONNECT_VERSION_PATCH}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.kdeconnect"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
)
endif()

install(TARGETS kdeconnect-indicator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.nonplasma.desktop DESTINATION ${KDE_INSTALL_APPDIR})
