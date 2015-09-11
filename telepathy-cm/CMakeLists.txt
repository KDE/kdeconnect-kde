project(connectcm)

add_definitions(-DCONNECTCM_LIBRARY)

set(connectcm_SOURCES
    connection.cpp
    protocol.cpp
    textchannel.cpp
    kdeconnecttelepathyprotocolfactory.cpp
)

add_library(connectcm SHARED ${connectcm_SOURCES})

target_link_libraries(connectcm
PUBLIC
    Qt5::Core
    ${TELEPATHY_QT5_LIBRARIES}
    ${TELEPATHY_QT5_SERVICE_LIBRARIES}
)

install (TARGETS connectcm
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

