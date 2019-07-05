#!/usr/bin/env bash

$XGETTEXT -L Python `find . -name '*.py'` -o $podir/kdeconnect-nautilus-extension.pot
