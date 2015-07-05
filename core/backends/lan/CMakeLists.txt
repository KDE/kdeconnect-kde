
set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}

    backends/lan/server.cpp
    backends/lan/lanlinkprovider.cpp
    backends/lan/landevicelink.cpp
    backends/lan/lanpairinghandler.cpp
    backends/lan/uploadjob.cpp
    backends/lan/downloadjob.cpp
    backends/lan/socketlinereader.cpp

    PARENT_SCOPE
)
