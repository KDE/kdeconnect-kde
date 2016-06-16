project(connectcm)

add_executable(connectcm
    main.cpp
    connection.cpp
    protocol.cpp
    textchannel.cpp
    kdeconnecttelepathyprotocolfactory.cpp
)

target_link_libraries(connectcm
    Qt5::Core
    ${TELEPATHY_QT5_LIBRARIES}
    ${TELEPATHY_QT5_SERVICE_LIBRARIES}
)

install (TARGETS connectcm DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

file(WRITE ${CMAKE_BINARY_DIR}/org.freedesktop.Telepathy.ConnectionManager.kdeconnect.service "Name=org.freedesktop.Telepathy.ConnectionManager.kdeconnect
Exec=$<TARGET_FILE_DIR:connectcm>")

install(FILES ${CMAKE_BINARY_DIR}/org.freedesktop.Telepathy.ConnectionManager.kdeconnect.service DESTINATION share/dbus-1/services)

install(
    FILES kdeconnect.manager
    DESTINATION share/telepathy/managers
)
