# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

function(systemctl_enable unit wantedby dir)
    set(wantedby_directory $ENV{DESTDIR}/${dir}/${wantedby}.wants/)
    file(MAKE_DIRECTORY ${wantedby_directory})
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ../${unit} ${wantedby_directory}/${unit}
        RESULT_VARIABLE enable_result
        ERROR_VARIABLE enable_fail
        COMMAND_ECHO STDOUT)
    if(NOT enable_result EQUAL "0")
        message(FATAL_ERROR "Systemctl failed: ${enable_fail} ${ARGN}")
    endif()
endfunction()
