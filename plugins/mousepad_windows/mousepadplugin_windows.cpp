/**
 * Copyright 2014 Ahmed I. Khalil <ahmedibrahimkhali@gmail.com>
 * Copyright 2015 Martin Gräßlin <mgraesslin@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mousepadplugin_windows.h"
#include <KPluginFactory>
#include <KLocalizedString>
#include <QDebug>
#include <QGuiApplication>
#include <QCursor>

#include <Windows.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_mousepad.json", registerPlugin< MousepadPlugin >(); )
  
MousepadPlugin::MousepadPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

MousepadPlugin::~MousepadPlugin()
{
}

bool MousepadPlugin::receivePackage(const NetworkPackage& np)
{
    float dx = np.get<float>(QStringLiteral("dx"), 0);
    float dy = np.get<float>(QStringLiteral("dy"), 0);

    bool isSingleClick = np.get<bool>(QStringLiteral("singleclick"), false);
    bool isDoubleClick = np.get<bool>(QStringLiteral("doubleclick"), false);
    bool isMiddleClick = np.get<bool>(QStringLiteral("middleclick"), false);
    bool isRightClick = np.get<bool>(QStringLiteral("rightclick"), false);
    bool isSingleHold = np.get<bool>(QStringLiteral("singlehold"), false);
    bool isSingleRelease = np.get<bool>(QStringLiteral("singlerelease"), false);
    bool isScroll = np.get<bool>(QStringLiteral("scroll"), false);
    QString key = np.get<QString>(QStringLiteral("key"), QLatin1String(""));
    int specialKey = np.get<int>(QStringLiteral("specialKey"), 0);

    if (isSingleClick || isDoubleClick || isMiddleClick || isRightClick || isSingleHold || isScroll || !key.isEmpty() || specialKey) {

		INPUT input={0};
		input.type = INPUT_MOUSE;

        if (isSingleClick) {
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			::SendInput(1,&input,sizeof(INPUT));
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			::SendInput(1,&input,sizeof(INPUT));
        } else if (isDoubleClick) {
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			::SendInput(1,&input,sizeof(INPUT));
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			::SendInput(1,&input,sizeof(INPUT));
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			::SendInput(1,&input,sizeof(INPUT));
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			::SendInput(1,&input,sizeof(INPUT));
		} else if (isMiddleClick) {
			input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
			::SendInput(1,&input,sizeof(INPUT));
			input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
			::SendInput(1,&input,sizeof(INPUT));
        } else if (isRightClick) {
			input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			::SendInput(1,&input,sizeof(INPUT));
			input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			::SendInput(1,&input,sizeof(INPUT));
        } else if (isSingleHold){
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			::SendInput(1,&input,sizeof(INPUT));
        } else if (isSingleRelease){
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			::SendInput(1,&input,sizeof(INPUT));
        } else if (isScroll) {
			input.mi.dwFlags = MOUSEEVENTF_WHEEL;
			input.mi.mouseData = dy;
			::SendInput(1,&input,sizeof(INPUT));
/*
		} else if (!key.isEmpty() || specialKey) {

            bool ctrl = np.get<bool>(QStringLiteral("ctrl"), false);
            bool alt = np.get<bool>(QStringLiteral("alt"), false);
            bool shift = np.get<bool>(QStringLiteral("shift"), false);

            if (ctrl) XTestFakeKeyEvent (display, XKeysymToKeycode(display, XK_Control_L), True, 0);
            if (alt) XTestFakeKeyEvent (display, XKeysymToKeycode(display, XK_Alt_L), True, 0);
            if (shift) XTestFakeKeyEvent (display, XKeysymToKeycode(display, XK_Shift_L), True, 0);

            if (specialKey)
            {
                if (specialKey >= (int)arraySize(SpecialKeysMap)) {
                    qWarning() << "Unsupported special key identifier";
                    return false;
                }

                int keycode = XKeysymToKeycode(display, SpecialKeysMap[specialKey]);

                XTestFakeKeyEvent (display, keycode, True, 0);
                XTestFakeKeyEvent (display, keycode, False, 0);

            } else {

                if (!m_fakekey) {
                    m_fakekey = fakekey_init(display);
                    if (!m_fakekey) {
                        qWarning() << "Failed to initialize libfakekey";
                        return false;
                    }
                }

                //We use fakekey here instead of XTest (above) because it can handle utf characters instead of keycodes.
                for (int i=0;i<key.length();i++) {
                    QByteArray utf8 = QString(key.at(i)).toUtf8();
                    fakekey_press(m_fakekey, (const uchar*)utf8.constData(), utf8.size(), 0);
                    fakekey_release(m_fakekey);
                }
            }

            if (ctrl) XTestFakeKeyEvent (display, XKeysymToKeycode(display, XK_Control_L), False, 0);
            if (alt) XTestFakeKeyEvent (display, XKeysymToKeycode(display, XK_Alt_L), False, 0);
            if (shift) XTestFakeKeyEvent (display, XKeysymToKeycode(display, XK_Shift_L), False, 0);
*/
        }

    } else { //Is a mouse move event
        QPoint point = QCursor::pos();
        QCursor::setPos(point.x() + (int)dx, point.y() + (int)dy);
    }
    return true;
}

#include "mousepadplugin_windows.moc"
