#!/usr/bin/env bash

#.qml
$XGETTEXT `find package -name '*.qml'` -L Java -o $podir/plasma_applet_kdeconnect.pot

#.ui (-j passed to merge into existing file)
$EXTRACTRC `find -name '*.ui' -o -name '*.rc'` >> rc.cpp
$XGETTEXT rc.cpp -o -j $podir/plasma_applet_kdeconnect.pot
rm -f rc.cpp
