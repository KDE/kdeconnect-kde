include_directories(${CMAKE_SOURCE_DIR})
add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kio")

set(kio_kdeconnect_PART_SRCS
   kiokdeconnect.cpp
)

add_library(kio_kdeconnect MODULE ${kio_kdeconnect_PART_SRCS})

target_link_libraries(kio_kdeconnect
    kdeconnectinterfaces
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    KF5::KIOCore
    KF5::I18n
)

########### install files ###############
install(TARGETS kio_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.protocol DESTINATION ${SERVICES_INSTALL_DIR})
