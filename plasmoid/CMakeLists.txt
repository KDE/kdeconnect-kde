# SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

plasma_add_applet(org.kde.plasma.kdeconnect
    QML_SOURCES
        qml/Battery.qml
        qml/Clipboard.qml
        qml/CompactRepresentation.qml
        qml/Connectivity.qml
        qml/DeviceDelegate.qml
        qml/FindMyPhone.qml
        qml/FullRepresentation.qml
        qml/main.qml
        qml/RemoteCommands.qml
        qml/Sftp.qml
        qml/Share.qml
        qml/SMS.qml
        qml/VirtualMonitor.qml
    GENERATE_APPLET_CLASS
)
