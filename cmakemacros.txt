
include(KDE4Defaults)
macro (generate_and_install_dbus_interface main_project_target header_file output_xml_file) #OPTIONS qdbus_options
    QT4_EXTRACT_OPTIONS(
        extra_files_ignore
        qdbus_options
        ${ARGN}
    )
    qt4_generate_dbus_interface(
        ${header_file}
        ${output_xml_file}
        OPTIONS ${qdbus_options}
    )
    add_custom_target(
        ${output_xml_file}
        SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${output_xml_file}
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${output_xml_file}
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
    )
    add_dependencies(
        ${main_project_target}
        ${output_xml_file}
    )
endmacro (generate_and_install_dbus_interface)
