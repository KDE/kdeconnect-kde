
set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}

    backends/lan/server.cpp
    backends/lan/lanlinkprovider.cpp
    backends/lan/landevicelink.cpp
    backends/lan/compositeuploadjob.cpp
    backends/lan/uploadjob.cpp
    backends/lan/mdnsdiscovery.cpp

    PARENT_SCOPE
)
