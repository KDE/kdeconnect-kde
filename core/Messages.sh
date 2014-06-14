#!/usr/bin/env bash

$EXTRACTRC `find -name '*.ui' -o -name '*.rc'` >> rc.cpp
$XGETTEXT rc.cpp -o $podir/kdeconnect-kded.pot
rm -f rc.cpp

#.cpp (-j passed to merge into existing file)
$XGETTEXT `find . -name '*.cpp'` -j -o $podir/kdeconnect-kded.pot

#.desktop and .notifyrc files doesn't need to be included here
