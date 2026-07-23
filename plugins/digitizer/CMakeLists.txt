# SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
# SPDX-License-Identifier: BSD-2-Clause

kdeconnect_add_plugin(kdeconnect_digitizer SOURCES digitizerplugin.cpp abstractdigitizerimpl.cpp)

target_link_libraries(kdeconnect_digitizer
    kdeconnectcore
    KF6::I18n
    KF6::WidgetsAddons
    PkgConfig::Evdev
)

target_sources(kdeconnect_digitizer PUBLIC linuxdigitizerimpl.cpp)

install(FILES 40-kdeconnect-uinput.rules DESTINATION ${KDE_INSTALL_LIBDIR}/udev/rules.d)
