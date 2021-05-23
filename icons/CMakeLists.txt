
set(APP_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/app/sc-apps-kdeconnect.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/app/sc-apps-kdeconnectindicator.svg)

list(APPEND APP_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/app/sc-apps-kdeconnectindicatordark.svg)

if(APPLE)
    # Craft adds share/icons into blacklist, which will not be copied into the bundle
    # As a workaround, on macOS, set ICON_INSTALL_DIR to another path
    set(ICON_INSTALL_DIR share/kdeconnect-icons)
endif()

ecm_install_icons(ICONS ${APP_ICONS} DESTINATION ${ICON_INSTALL_DIR})
