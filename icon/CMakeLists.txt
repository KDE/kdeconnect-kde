
set(APP_ICONS
    256-apps-kdeconnect.png
    128-apps-kdeconnect.png
    32-apps-kdeconnect.png
    64-apps-kdeconnect.png
    48-apps-kdeconnect.png
    22-apps-kdeconnect.png
    16-apps-kdeconnect.png
    sc-apps-kdeconnect.svgz
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
    32-status-laptopconnected.png
    32-status-laptopdisconnected.png
    32-status-laptoptrusted.png
    32-status-smartphoneconnected.png
    32-status-smartphonedisconnected.png
    32-status-smartphonetrusted.png
    32-status-tabletconnected.png
    32-status-tabletdisconnected.png
    32-status-tablettrusted.png
    32-status-tvconnected.png
    32-status-tvdisconnected.png
    32-status-tvtrusted.png
    sc-status-laptopconnected.svg
    sc-status-laptopdisconnected.svg
    sc-status-laptoptrusted.svg
    sc-status-smartphoneconnected.svg
    sc-status-smartphonedisconnected.svg
    sc-status-smartphonetrusted.svg
    sc-status-tabletconnected.svg
    sc-status-tabletdisconnected.svg
    sc-status-tablettrusted.svg
    sc-status-tvconnected.svg
    sc-status-tvdisconnected.svg
    sc-status-tvtrusted.svg
DESTINATION
    ${ICON_INSTALL_DIR}
)

