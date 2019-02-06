import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.5 as Kirigami
import Qt.labs.platform 1.1
import org.kde.kdeconnect 1.0

Kirigami.FormLayout {

    property string device

    function apply() {
        config.set("ringtone", path.text)
    }

    FileDialog {
        id: fileDialog
        currentFile: path.text

        onAccepted: {
            path.text = currentFile.toString().replace("file://", "")
        }
    }

    KdeConnectPluginConfig {
        id: config
        deviceId: device
        pluginName: "kdeconnect_findthisdevice"

        onConfigChanged: {
            path.text = get("ringtone", StandardPaths.writableLocation(StandardPaths.DownloadsLocation).toString().replace("file://", ""))
        }
    }

    RowLayout {
        Kirigami.FormData.label: i18n("Sound to play:")

        TextField {
            id: path
        }

        Button {
            icon.name: "document-open"
            onClicked: {
                fileDialog.open()
            }
        }
    }
}

