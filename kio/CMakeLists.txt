include_directories(${CMAKE_SOURCE_DIR})
add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kio")

set(kio_kdeconnect_PART_SRCS
   kiokdeconnect.cpp
)

add_library(kio_kdeconnect MODULE ${kio_kdeconnect_PART_SRCS})

target_link_libraries(kio_kdeconnect
    kdeconnectinterfaces
    Qt5::Core
    Qt5::Network
    KF5::KIOCore
    KF5::I18n
)

########### install files ###############
set_target_properties(kio_kdeconnect PROPERTIES OUTPUT_NAME "kdeconnect")
set_target_properties(kio_kdeconnect PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio")
install(TARGETS kio_kdeconnect DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
