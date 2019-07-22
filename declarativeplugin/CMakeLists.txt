include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})

set(kdeconnectdeclarativeplugin_SRC
    kdeconnectdeclarativeplugin.cpp
    responsewaiter.cpp
    objectfactory.cpp
)

qt5_add_resources(kdeconnectdeclarativeplugin_SRC resources.qrc)

add_library(kdeconnectdeclarativeplugin SHARED ${kdeconnectdeclarativeplugin_SRC})
target_link_libraries(kdeconnectdeclarativeplugin
    Qt5::Qml
    kdeconnectinterfaces
)

install(TARGETS kdeconnectdeclarativeplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)

# So qml apps using this plugin can be run from build/bin directory
add_custom_target(copy_to_build_dir_bin)
add_custom_command(TARGET copy_to_build_dir_bin POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E
                   copy ${CMAKE_CURRENT_SOURCE_DIR}/qmldir ${CMAKE_BINARY_DIR}/bin/org/kde/kdeconnect/)
add_dependencies(kdeconnectdeclarativeplugin copy_to_build_dir_bin)
set_target_properties(kdeconnectdeclarativeplugin
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/org/kde/kdeconnect/
)
