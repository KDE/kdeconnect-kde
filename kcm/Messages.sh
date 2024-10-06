#!/usr/bin/env bash

$EXTRACTRC `find -name '*.ui' -o -name '*.rc'` >> rc.cpp
$XGETTEXT rc.cpp -o $podir/kdeconnect-kcm.pot
rm -f rc.cpp

#.cpp (-j passed to merge into existing file)
$XGETTEXT `find . -name '*.qml'` -j -o $podir/kdeconnect-kcm.pot
$XGETTEXT `find . -name '*.cpp'` -j -o $podir/kdeconnect-kcm.pot

