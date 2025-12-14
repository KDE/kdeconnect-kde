
set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}
    backends/lan/server.cpp
    backends/lan/lanlinkprovider.cpp
    backends/lan/landevicelink.cpp
    backends/lan/compositeuploadjob.cpp
    backends/lan/uploadjob.cpp
)

if(NOT APPLE AND NOT WIN32)
    set(backends_kdeconnect_SRCS
        ${backends_kdeconnect_SRCS}
        backends/lan/avahidiscovery.cpp
    )
else()
    set(backends_kdeconnect_SRCS
        ${backends_kdeconnect_SRCS}
        backends/lan/mdnshdiscovery.cpp
        backends/lan/mdnsh_wrapper.cpp
    )
endif()

set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}
    PARENT_SCOPE
)
