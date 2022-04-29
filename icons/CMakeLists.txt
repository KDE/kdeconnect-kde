
set(APP_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/app/sc-apps-kdeconnect.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/app/sc-apps-kdeconnectindicator.svg)

list(APPEND APP_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/app/sc-apps-kdeconnectindicatordark.svg)

if(APPLE)
    # Craft adds share/icons into blacklist, which will not be copied into the bundle
    # As a workaround, on macOS, set KDE_INSTALL_ICONDIR to another path
    set(KDE_INSTALL_ICONDIR share/kdeconnect-icons)
endif()

ecm_install_icons(ICONS ${APP_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR})

ecm_install_icons(ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-laptopconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-laptopdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-laptoptrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-smartphoneconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-smartphonedisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-smartphonetrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-tabletconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-tabletdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-tablettrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-tvconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-tvdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/16-status-tvtrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-laptopconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-laptopdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-laptoptrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-smartphoneconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-smartphonedisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-smartphonetrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-tabletconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-tabletdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-tablettrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-tvconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-tvdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/22-status-tvtrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-laptopconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-laptopdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-laptoptrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-smartphoneconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-smartphonedisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-smartphonetrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-tabletconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-tabletdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-tablettrusted.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-tvconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-tvdisconnected.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/status/32-status-tvtrusted.svg
    DESTINATION ${KDE_INSTALL_ICONDIR})
