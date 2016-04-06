# KDE Connect - desktop app

KDE Connect is a multi-platform app that allows your devices to communicate (eg: your phone and your computer).

## (Some) Features
- **Shared clipboard**: copy and paste between your phone and your computer (or any other device).
- **Notification sync**: Read your Android notifications from the desktop.
- **Share files and URLs** instantly from one device to another.
- **Multimedia remote control**: Use your phone as a remote for Linux media players.
- **Virtual touchpad**: Use your phone screen as your computer's touchpad.

All this without wires, over the already existing WiFi network, and using a secure, encrypted protocol.

## Supported platforms
- Computers running Plasma 5, KDE4, Unity (Ubuntu), Gnome 3, Elementary OS...
- Android, by installing the [KDE Connect Android app](https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp) (also available on [F-Droid](https://f-droid.org/repository/browse/?fdid=org.kde.kdeconnect_tp)).

There is also source code for an unmaintained iOS port, waiting for somebody to give it some love :)

## How to install
This explains how to install KDE Connect on your computer. You will also need to install it in your phone and pair both devices in the app if you want it to be any useful.

### On Linux
Look in your distribution repo for a package called `kdeconnect-kde`, `kdeconnect-plasma`, or just `kdeconnect`. If it's not there and you know how to build software from sources, you just found the repo :)

If you are not using Plasma 5 or KDE4, you will also need to install [indicator-kdeconnect](https://github.com/vikoadi/indicator-kdeconnect) (available as an [Ubuntu PPA](https://code.launchpad.net/~vikoadi/+archive/ubuntu/ppa/)) for integration with other desktops using appindicator.

### On Mac or Windows
There is no support for Mac or Windows yet. The last time I checked it was compiling on Windows, so it's only lacking a user interface. Same for Mac. Contributions welcome!

### On BSD
It should work, but no promises.

## How does it work?
KDE Connect consists of an UI-agnostic "core" library which exposes a series of DBus interfaces, and several UI components that consume these DBus interfaces. This way, new UI components can be added to integrate better with specific platforms or desktops, without having to reimplement the protocol or any of the internals. The core KDE Connect library is also divided in 4 big blocks:

- **LinkProviders**: Are in charge of discovering other KDE Connect-enabled devices in the network and establishing a Link to them.
- **Devices**: Represent a remote device, abstracting the specific Link that is being used to reach it.
- **NetworkPackets**: JSON-serializable and self-contained pieces of information to be sent by the plugins between devices.
- **Plugins**: Independent pieces of code which implement a specific feature. Plugins will use NetworkPackets to exchange information through the network with other Plugins on a remote Device.

The basic structure of a NetworkPacket (before encryption) is the following:

```
{
  "id": 123456789,
  "type": "com.example.myplugin",
  "body": {  },
  "version": 5
}
```

The content of the `"body"` section is defined by each Plugin. Hence, only the emisor and receiver plugins of a given packet type need agree on the contents of the body.

NetworkPackets can also have binary data attached that can't be serialized to JSON. In this case, two new fields will be added:

`"payloadSize"`: The size of the file, or -1 if it is a stream without known size.  
`"payloadTransferInfo"`: Another JSON object where the specific Link can add information so the Link in the remote end can establish a connection and receive the payload (eg: IP and port in a local network). It's up to the Link implementation to decide how to use this field.

## License
[GNU GPL v2](https://www.gnu.org/licenses/gpl-2.0.html) and [GNU GPL v3](https://www.gnu.org/licenses/gpl-3.0.html)

If you are reading this from Github, you should know that this is just a mirror of the [KDE Project repo](https://projects.kde.org/projects/extragear/network/kdeconnect-kde/repository/).

[![Build Status](https://build.kde.org/buildStatus/icon?job=kdeconnect-kde master kf5-qt5)](https://build.kde.org/job/kdeconnect-kde%20master%20kf5-qt5/)
