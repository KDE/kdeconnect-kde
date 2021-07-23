add_library(kdeconnectdeclarativeplugin SHARED
    kdeconnectdeclarativeplugin.cpp
    responsewaiter.cpp
    objectfactory.cpp
    pointerlocker.cpp
    resources.qrc
)
target_link_libraries(kdeconnectdeclarativeplugin
    Qt5::Qml
    kdeconnectinterfaces
    kdeconnectcore
)

if (TARGET KF5::WaylandClient)
    target_sources(kdeconnectdeclarativeplugin PRIVATE pointerlockerwayland.cpp)
    target_link_libraries(kdeconnectdeclarativeplugin KF5::WaylandClient)
    target_compile_definitions(kdeconnectdeclarativeplugin PRIVATE -DWITH_WAYLAND=1)
else()
    target_compile_definitions(kdeconnectdeclarativeplugin PRIVATE -DWITH_WAYLAND=0)
endif()

install(TARGETS kdeconnectdeclarativeplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)

ecm_generate_qmltypes(org.kde.kdeconnect 1.0 DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kdeconnect)

# So qml apps using this plugin can be run from build/bin directory
add_custom_target(copy_to_build_dir_bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/org/kde/kdeconnect)
add_custom_command(TARGET copy_to_build_dir_bin
                   COMMAND ${CMAKE_COMMAND} -E
                   copy ${CMAKE_CURRENT_SOURCE_DIR}/qmldir ${CMAKE_BINARY_DIR}/bin/org/kde/kdeconnect/)
add_dependencies(kdeconnectdeclarativeplugin copy_to_build_dir_bin)
set_target_properties(kdeconnectdeclarativeplugin
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/org/kde/kdeconnect/
)
