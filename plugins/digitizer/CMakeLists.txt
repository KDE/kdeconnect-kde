# SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
# SPDX-License-Identifier: BSD-2-Clause

if(UNIX AND NOT APPLE)
    pkg_check_modules(Evdev IMPORTED_TARGET libevdev)

    if (Evdev_FOUND)
        set(HAS_DEPS 1)
    endif()
endif()

if(HAS_DEPS)
    kdeconnect_add_plugin(kdeconnect_digitizer SOURCES digitizerplugin.cpp abstractdigitizerimpl.cpp)

    target_link_libraries(kdeconnect_digitizer
        kdeconnectcore
        KF6::I18n
        KF6::WidgetsAddons
    )

    if(UNIX AND NOT APPLE)
        target_link_libraries(kdeconnect_digitizer PkgConfig::Evdev)

        target_sources(kdeconnect_digitizer PUBLIC linuxdigitizerimpl.cpp)
    endif()

    install(FILES 40-kdeconnect-uinput.rules DESTINATION ${KDE_INSTALL_LIBDIR}/udev/rules.d)

    message(STATUS "digitizer: Dependencies met, compiling.")
else()
    message(WARNING "digitizer: Dependencies missing, not compiling.")
endif()
