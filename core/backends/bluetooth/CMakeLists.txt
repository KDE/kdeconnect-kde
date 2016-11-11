
set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}

    backends/bluetooth/bluetoothlinkprovider.cpp
    backends/bluetooth/bluetoothdevicelink.cpp
    backends/bluetooth/bluetoothpairinghandler.cpp
    backends/bluetooth/bluetoothdownloadjob.cpp
    backends/bluetooth/bluetoothuploadjob.cpp

    PARENT_SCOPE
)
