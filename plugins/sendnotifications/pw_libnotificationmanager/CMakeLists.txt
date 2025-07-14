if(NOT WIN32)
target_sources(kdeconnect_sendnotifications PRIVATE
    notification.cpp
)
endif()
