# -*- coding: UTF-8 -*-

"""
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Andy Holmes <andrew.g.r.holmes@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
"""

__author__ = "Albert Vaca Cintora <albertvaka@gmail.com>"
__version__ = "1.0"
__appname__ = "kdeconnect-share"
__app_disp_name__ = "Share files to your phone via KDE Connect"
__website__ = "https://community.kde.org/KDEConnect"

import gettext
from functools import partial

from gi.repository import Nautilus, Gio, GLib, GObject

try:
    i18n = gettext.translation('kdeconnect-nautilus-extension')
    _ = i18n.gettext
except (IOError, OSError) as e:
    print('kdeconnect-nautilus: {0}'.format(e))
    i18n = gettext.translation('kdeconnect-nautilus-extension', fallback=True)
    _ = i18n.gettext

class KdeConnectShareExtension(GObject.GObject, Nautilus.MenuProvider):
    """A context menu for sending files via KDE Connect."""

    def refresh_devices_list(self, *args, **kwargs):
        try:
            onlyReachable = True
            onlyPaired = True
            variant = GLib.Variant('(bb)', (onlyReachable, onlyPaired))
            devices = self.dbus.call_sync('deviceNames', variant, 0, -1, None)
            self.devices = devices.unpack()[0]
        except Exception as e:
            raise Exception('Error while getting reachable devices')


    def __init__(self):
        GObject.GObject.__init__(self)

        self.devices = {}

        self.dbus = Gio.DBusProxy.new_for_bus_sync(
            Gio.BusType.SESSION,
            Gio.DBusProxyFlags.NONE,
            None,
            'org.kde.kdeconnect',
            '/modules/kdeconnect',
            'org.kde.kdeconnect.daemon',
            None)

        connection = Gio.bus_get_sync(
            Gio.BusType.SESSION,
            None)
        connection.signal_subscribe(
            None,
            'org.kde.kdeconnect.daemon',
            'deviceListChanged',
            "/modules/kdeconnect",
            None,
            Gio.DBusSignalFlags.NONE,
            partial(self.refresh_devices_list, self),
        )

        self.refresh_devices_list()

    def send_files(self, menu, files, deviceId):
        device_proxy = Gio.DBusProxy.new_for_bus_sync(
            Gio.BusType.SESSION,
            Gio.DBusProxyFlags.NONE,
            None,
            'org.kde.kdeconnect',
            '/modules/kdeconnect/devices/'+deviceId+'/share',
            'org.kde.kdeconnect.device.share',
            None)

        for file in files:
            variant = GLib.Variant('(s)', (file.get_uri(),))
            device_proxy.call_sync('shareUrl', variant, 0, -1, None)

    def get_file_items(self, *args):
        # `args` will be `[files: List[Nautilus.FileInfo]]` in Nautilus 4.0 API,
        # and `[window: Gtk.Widget, files: List[Nautilus.FileInfo]]` in Nautilus 3.0 API.
        files = args[-1]

        #We can only send regular files
        for uri in files:
            if uri.get_uri_scheme() != 'file' or uri.is_directory():
                return

        devices = self.devices

        if len(devices) == 0:
            return

        if len(devices) == 1:
            deviceId, deviceName = list(devices.items())[0]
            item = Nautilus.MenuItem(
                        name='KdeConnectShareExtension::Devices::' + deviceId,
                        label=_("Send to %s via KDE Connect") % deviceName,
                    )
            item.connect('activate', self.send_files, files, deviceId)
            return item,
        else:
            menu = Nautilus.MenuItem(
                name='KdeConnectShareExtension::Devices',
                label=_('Send via KDE Connect'),
            )
            submenu = Nautilus.Menu()
            menu.set_submenu(submenu)

            for deviceId, deviceName in devices.items():
                item = Nautilus.MenuItem(
                    name='KdeConnectShareExtension::Devices::' + deviceId,
                    label=deviceName,
                )
                item.connect('activate', self.send_files, files, deviceId)
                submenu.append_item(item)

            return menu,

