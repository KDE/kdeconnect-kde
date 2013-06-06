#!/bin/sh

. ~/.bashrc

KDE_BUILD_CONFIRMATION=false

cs android-connect

if kdebuild; then


	killall kded4
	while killall -9 kded4; do
		true
	done
	
	#qdbus org.kde.kded /kded unloadModule androidshine
	#qdbus org.kde.kded /kded loadModule androidshine
	kded4 2>&1 | grep -v "^kded(" &

	echo ""

fi


