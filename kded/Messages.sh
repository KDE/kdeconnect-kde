#!/usr/bin/env bash

$XGETTEXT `find . -name '*.cpp'` -o $podir/kdeconnect-kded.pot

#.desktop and .notifyrc files doesn't need to be included here
