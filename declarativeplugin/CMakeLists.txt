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


##
# Installs a QML declarative plugin and its qmldir file
#
function(install_qml_plugin plugin_name qml_install_dir_path)

    install(TARGETS ${plugin_name} DESTINATION ${QML_INSTALL_DIR}/${qml_install_dir_path})
    install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/${qml_install_dir_path})

    # So qml apps using this plugin can be run from build/bin directory
    add_custom_target(copy_to_build_dir_bin)
    add_custom_command(TARGET copy_to_build_dir_bin POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E
                    copy ${CMAKE_CURRENT_SOURCE_DIR}/qmldir ${CMAKE_BINARY_DIR}/bin/${qml_install_dir_path})
    add_dependencies(${plugin_name} copy_to_build_dir_bin)
    set_target_properties(${plugin_name}
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${qml_install_dir_path}
    )
endfunction()

install_qml_plugin(kdeconnectdeclarativeplugin "org/kde/kdeconnect")

