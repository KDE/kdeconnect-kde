import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.5 as Kirigami
import Qt.labs.platform 1.1
import org.kde.kdeconnect 1.0

Kirigami.FormLayout {

    property string device

    property var action: Kirigami.Action {
        icon.name: "dialog-ok"
        text: i18n("Apply")
        onTriggered: config.set("incoming_path", path.text)
    }

    FolderDialog {
        id: folderDialog
        currentFolder: path.text

        onAccepted: {
            path.text = currentFolder.toString().replace("file://", "")
        }
    }

    KdeConnectPluginConfig {
        id: config
        deviceId: device
        pluginName: "kdeconnect_share"

        onConfigChanged: {
            path.text = get("incoming_path", StandardPaths.writableLocation(StandardPaths.DownloadsLocation).toString().replace("file://", ""))
        }
    }

    RowLayout {
        Kirigami.FormData.label: i18n("Save files in:")

        TextField {
            id: path
        }

        Button {
            icon.name: "document-open"
            onClicked: folderDialog.open()
        }
    }

    Label {
        text: "%1 in the path will be replaced with the specific device name"
    }
}
