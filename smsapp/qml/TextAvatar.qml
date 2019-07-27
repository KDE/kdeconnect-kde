/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2019 Kaidan developers and contributors
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

import org.kde.kirigami 2.2 as Kirigami
import QtQuick 2.7
import QtQuick.Layouts 1.2

Rectangle {
	id: avatar

	property string name

	color: Kirigami.Theme.highlightColor//Qt.lighter(kaidan.utils.getUserColor(name))
	radius: width * 0.5

	Text {
		id: text

		anchors.fill: parent
		anchors.margins: Kirigami.Units.devicePixelRatio * 5

		property var model

		renderType: Text.QtRendering
		color: Kirigami.Theme.textColor

		font.weight: Font.Bold
		font.pointSize: 100
		minimumPointSize: Kirigami.Theme.defaultFont.pointSize
		fontSizeMode: Text.Fit

		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter

		text: {
		        if (!name) {
		            return ""
		        }

			// WIPQTQUICK HACK TODO Probably doesn't work with non-latin1.
			var match = name.match(/([a-zA-Z0-9])([a-zA-Z0-9])/);
			var abbrev = match[1].toUpperCase();

			if (match.length > 2) {
				abbrev += match[2].toLowerCase();
			}

			return abbrev;
		}
	}
}
