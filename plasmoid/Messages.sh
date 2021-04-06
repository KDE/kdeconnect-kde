#!/usr/bin/env bash

#.qml
$XGETTEXT `find package -name '*.qml'` -o $podir/plasma_applet_org.kde.kdeconnect.pot
