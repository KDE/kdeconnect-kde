# KDE Connect - desktop app

KDE Connect is a multi-platform app that allows your devices to communicate (eg: your phone and your computer).

## (Some) Features
- **Shared clipboard**: copy and paste between your phone and your computer (or any other device).
- **Notification sync**: Read and reply to your Android notifications from the desktop.
- **Share files and URLs** instantly from one device to another including some filesystem integration.
- **Multimedia remote control**: Use your phone as a remote for Linux media players.
- **Virtual touchpad**: Use your phone screen as your computer's touchpad and keyboard.
- **Presentation remote**: Advance your presentation slides straight from your phone.

All this is done completely wirelessly, utilising TLS encryption.

## Supported platforms
- Computers running Linux with Plasma 5, Gnome 3, Elementary OS... any distro with Qt5 support given a little work :)
- Android, by installing the KDE Connect app from the [Play Store](https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp) or [F-Droid](https://f-droid.org/repository/browse/?fdid=org.kde.kdeconnect_tp).

## How to install
These instructions explain how to install KDE Connect on your computer. You will also need to install it on your Android device and pair them together in the app before using this application. The Android app repository can be found [here](https://invent.kde.org/network/kdeconnect-android)

### On Linux
Look in your distribution repo for a package called `kdeconnect-kde`, `kdeconnect-plasma`, or just `kdeconnect`. If it's not there and you know how to build software from sources, you just found the repo :), instructions are [here](https://community.kde.org/KDEConnect#Building_KDE_Connect_.28Desktop.29)

### On Mac or Windows
Platforms other than Linux are not officially supported, as there has yet to be an official release of KDE Connect for MacOS or Windows. However, most of the features have already been ported to windows so you can compile KDE Connect for Windows using [Craft](https://community.kde.org/Craft). It hasn't yet been completely tested on MacOS, contributions and feedback are welcome!

### On BSD
It should work, but no promises :)

## How does it work?
KDE Connect consists of a UI-agnostic "core" library that exposes a series of DBus interfaces, and several UI components that consume these DBus interfaces. This way, new UI components can be added to integrate better with specific platforms or desktops, without having to reimplement the protocol or any of the internals. The core KDE Connect library is also divided into 4 big blocks:

- **LinkProviders**: Are in charge of discovering other KDE Connect-enabled devices in the network and establishing a Link to them.
- **Devices**: Represent a remote device, abstracting the specific Link that is being used to reach it.
- **NetworkPackets**: JSON-serializable and self-contained pieces of information to be sent by the plugins between devices.
- **Plugins**: Independent pieces of code that implement a specific feature. Plugins will use NetworkPackets to exchange information through the network with other Plugins on a remote Device.

The basic structure of a NetworkPacket is the following:

```
{
  "id": 123456789,
  "type": "com.example.myplugin",
  "body": {  },
  "version": 5
}
```

The content of the `"body"` section is defined by each Plugin. Hence, only the emitter and receiver plugins of a given packet type need agree on the contents of the body.

NetworkPackets can also have binary data attached that can't be serialized to JSON. In this case, two new fields will be added:

`"payloadSize"`: The size of the file, or -1 if it is a stream without known size.
`"payloadTransferInfo"`: Another JSON object where the specific Link can add information so the Link in the remote end can establish a connection and receive the payload (eg: IP and port in a local network). It's up to the Link implementation to decide how to use this field.

## Contributing

To contribute patches, use [KDE Connect's GitLab](https://invent.kde.org/network/kdeconnect-kde). There you can also find a task list with stuff to do and links to other relevant resources. It is a good idea to also subscribe to the [KDE Connect mailing list](https://mail.kde.org/mailman/listinfo/kdeconnect). We can also be reached on IRC at #kdeconnect on irc.libera.chat or on [Telegram](https://t.me/joinchat/AOS6gA37orb2dZCLhqbZjg), contributions and feedback are warmly welcomed.

For bug reporting, please use [KDE's Bugzilla](https://bugs.kde.org). Please do not use the issue tracker in GitLab since we want to keep everything in one place.

Please know that all translations for all KDE apps are handled by the [localization team](https://l10n.kde.org/). If you would like to submit a translation, that should be done by working with the proper team for that language.

## License
[GNU GPL v2](https://www.gnu.org/licenses/gpl-2.0.html) and [GNU GPL v3](https://www.gnu.org/licenses/gpl-3.0.html)

If you are reading this from Github, you should know that this is just a mirror of the [KDE Project repo](https://invent.kde.org/network/kdeconnect-kde).
