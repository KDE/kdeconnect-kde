#!/bin/bash

#Source bashrc to define kdebuild and environment variables
#http://techbase.kde.org/Getting_Started/Build/Environment
. ~/.bashrc

KDE_BUILD_CONFIRMATION=false
export VERBOSE=1

if kdebuild; then

	echo "--------BUILD DONE--------------"

	kbuildsycoca4

	killall kded4 2> /dev/null
	while killall -9 kded4 2> /dev/null; do
		true
	done

	#qdbus org.kde.kded /kded unloadModule kdeconnect
	#qdbus org.kde.kded /kded loadModule kdeconnect

	echo "--------STARTING KDED--------------"

	kded4 --nofork # 2>&1 | grep -v "^kded(" &

fi

