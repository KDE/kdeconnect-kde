#!/bin/bash

#Source bashrc to define kdebuild and environment variables
#http://techbase.kde.org/Getting_Started/Build/Environment
. ~/.bashrc

KDE_BUILD_CONFIRMATION=false

if kdebuild; then

	killall kded4
	while killall -9 kded4; do
		true
	done

	#qdbus org.kde.kded /kded unloadModule androidshine
	#qdbus org.kde.kded /kded loadModule androidshine
	kded4 2>&1 | grep -v "^kded(" &

fi

