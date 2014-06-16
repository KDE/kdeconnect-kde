include_directories(${CMAKE_SOURCE_DIR})

set(kio_kdeconnect_PART_SRCS
   kiokdeconnect.cpp
   kdebugnamespace.cpp)

add_library(kio_kdeconnect MODULE ${kio_kdeconnect_PART_SRCS})

target_link_libraries(kio_kdeconnect
    KF5::KIOCore
    KF5::KDELibs4Support
    Qt5::Core
    Qt5::Gui
    kdeconnectinterfaces
)

########### install files ###############
install(TARGETS kio_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.protocol DESTINATION ${SERVICES_INSTALL_DIR})
