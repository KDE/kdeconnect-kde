
set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}

    backends/bluetooth/multiplexchannel.cpp
    backends/bluetooth/multiplexchannelstate.cpp
    backends/bluetooth/connectionmultiplexer.cpp
    backends/bluetooth/bluetoothlinkprovider.cpp
    backends/bluetooth/bluetoothdevicelink.cpp
    backends/bluetooth/bluetoothdownloadjob.cpp
    backends/bluetooth/bluetoothuploadjob.cpp
    backends/bluetooth/asynclinkprovider.cpp

    PARENT_SCOPE
)
