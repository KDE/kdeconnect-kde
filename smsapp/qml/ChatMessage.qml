/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2018 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.6
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0 as Controls
import org.kde.kirigami 2.0 as Kirigami

RowLayout {
	id: root

	property bool sentByMe: true
	property string messageBody
	property date dateTime
	property bool isRead: false
	property string recipientAvatarUrl

	// own messages are on the right, others on the left
	layoutDirection: sentByMe ? Qt.RightToLeft : Qt.LeftToRight
	spacing: Kirigami.Units.largeSpacing
	width: parent.width - Kirigami.Units.largeSpacing * 4
	anchors.horizontalCenter: parent.horizontalCenter

	RoundImage {
		id: avatar
		visible: !sentByMe
		source: recipientAvatarUrl
		fillMode: Image.PreserveAspectFit
		mipmap: true
		height: width
		Layout.preferredHeight: Kirigami.Units.gridUnit * 2.2
		Layout.preferredWidth: Kirigami.Units.gridUnit * 2.2
		Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
		sourceSize.height: Kirigami.Units.gridUnit * 2.2
		sourceSize.width: Kirigami.Units.gridUnit * 2.2
	}

	Rectangle {
		id: box
		Layout.preferredWidth: content.width + Kirigami.Units.gridUnit * 0.9
		Layout.preferredHeight: content.height + Kirigami.Units.gridUnit * 0.6

		color: sentByMe ? Kirigami.Theme.complementaryTextColor : Kirigami.Theme.highlightColor
		radius: Kirigami.Units.smallSpacing * 2

		layer.enabled: box.visible
		layer.effect: DropShadow {
			verticalOffset: Kirigami.Units.gridUnit * 0.08
			horizontalOffset: Kirigami.Units.gridUnit * 0.08
			color: Kirigami.Theme.disabledTextColor
			samples: 10
			spread: 0.1
		}
	}

	ColumnLayout {
		id: content
		spacing: 0
		anchors.centerIn: box

		Controls.Label {
			text: messageBody
			textFormat: Text.PlainText
			wrapMode: Text.Wrap
			color: sentByMe ? Kirigami.Theme.buttonTextColor : Kirigami.Theme.complementaryTextColor

			Layout.maximumWidth: root.width - Kirigami.Units.gridUnit * 6
		}

		RowLayout {
			Controls.Label {
				id: dateLabel
				text: Qt.formatDateTime(dateTime, "dd. MMM yyyy, hh:mm")
				color: Kirigami.Theme.disabledTextColor
				font.pixelSize: Kirigami.Units.gridUnit * 0.8
			}
		}
	}

	Item {
		Layout.fillWidth: true
	}
}
