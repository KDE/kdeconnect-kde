#!/usr/bin/env bash

$XGETTEXT `find . -name '*.cpp' -o -name '*.qml'` -o $podir/kdeconnect-app.pot
