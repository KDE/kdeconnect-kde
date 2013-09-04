#!/usr/bin/env bash

#.qml
$XGETTEXT `find package -name '*.qml'` -L Java -o $podir/kdeconnect-plasmoid.pot

#.ui (-j passed to merge into existing file)
$EXTRACTRC `find -name '*.ui' -o -name '*.rc'` >> rc.cpp
$XGETTEXT rc.cpp -o -j $podir/kdeconnect-plasmoid.pot
rm -f rc.cpp


