
set(APP_ICONS
    sc-apps-kdeconnect.svg
    sc-apps-kdeconnectindicator.svg
)

if(WIN32)
    list(APPEND APP_ICONS sc-apps-kdeconnectindicatorwin.svg)
else()
    list(APPEND APP_ICONS sc-apps-kdeconnectindicatordark.svg)
endif()

ecm_install_icons(
ICONS
    ${APP_ICONS}
DESTINATION
    ${ICON_INSTALL_DIR}
)

ecm_install_icons(
ICONS
    16-status-laptopconnected.svg
    16-status-laptopdisconnected.svg
    16-status-laptoptrusted.svg
    16-status-smartphoneconnected.svg
    16-status-smartphonedisconnected.svg
    16-status-smartphonetrusted.svg
    16-status-tabletconnected.svg
    16-status-tabletdisconnected.svg
    16-status-tablettrusted.svg
    16-status-tvconnected.svg
    16-status-tvdisconnected.svg
    16-status-tvtrusted.svg
    22-status-laptopconnected.svg
    22-status-laptopdisconnected.svg
    22-status-laptoptrusted.svg
    22-status-smartphoneconnected.svg
    22-status-smartphonedisconnected.svg
    22-status-smartphonetrusted.svg
    22-status-tabletconnected.svg
    22-status-tabletdisconnected.svg
    22-status-tablettrusted.svg
    22-status-tvconnected.svg
    22-status-tvdisconnected.svg
    22-status-tvtrusted.svg
    32-status-laptopconnected.svg
    32-status-laptopdisconnected.svg
    32-status-laptoptrusted.svg
    32-status-smartphoneconnected.svg
    32-status-smartphonedisconnected.svg
    32-status-smartphonetrusted.svg
    32-status-tabletconnected.svg
    32-status-tabletdisconnected.svg
    32-status-tablettrusted.svg
    32-status-tvconnected.svg
    32-status-tvdisconnected.svg
    32-status-tvtrusted.svg
DESTINATION
    ${ICON_INSTALL_DIR}
)

