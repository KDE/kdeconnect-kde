#!/usr/bin/env bash

$EXTRACTRC `find . -name '*.ui' -o -name '*.rc'` >> rc.cpp
$XGETTEXT `find . -name '*.cpp' -o -name '*.qml'` -o $podir/kdeconnect-plugins.pot
