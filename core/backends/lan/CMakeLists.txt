
set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}
    backends/lan/server.cpp
    backends/lan/lanlinkprovider.cpp
    backends/lan/landevicelink.cpp
    backends/lan/compositeuploadjob.cpp
    backends/lan/uploadjob.cpp
    backends/lan/mdnshdiscovery.cpp
    backends/lan/mdnsh_wrapper.cpp
)

if(UNIX AND NOT APPLE)
    set(backends_kdeconnect_SRCS
        ${backends_kdeconnect_SRCS}
        backends/lan/avahidiscovery.cpp
    )
endif()

set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}
    PARENT_SCOPE
)
