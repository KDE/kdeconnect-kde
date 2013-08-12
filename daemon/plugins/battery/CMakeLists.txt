
generate_and_install_dbus_interface(
    kded_kdeconnect
    packageinterfaces/devicebatteryinformation_p.h
    org.kde.kdeconnect.device.battery.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kded_kdeconnect
    packageinterfaces/batterypackageinterface.h
    org.kde.kdeconnect.battery.xml
    OPTIONS -a
)
