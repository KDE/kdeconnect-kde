
set(kded_kdeconnect_SRCS
    ${kded_kdeconnect_SRCS}

    backends/lan/lanlinkprovider.cpp
    backends/lan/landevicelink.cpp
    backends/lan/uploadjob.cpp
    backends/lan/downloadjob.cpp
    backends/lan/socketlinereader.cpp

    PARENT_SCOPE
)
