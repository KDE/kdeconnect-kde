/**
 * SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Window 2.5
import QtQuick.Particles 2.0

Item {
    id: root
    anchors.fill: parent

    property real xPos: 0.5
    property real yPos: 0.5

    property real devicePixelRatio: Window.window != null && Window.window.screen != null ? Window.window.screen.devicePixelRatio : 1.0

    ParticleSystem {
        id: particles
        width: parent.width/20
        height: width

        x: root.width * root.xPos - width/2
        y: root.height * root.yPos - height/2

        ImageParticle {
            groups: ["center"]
            anchors.fill: parent
            source: "qrc:///particleresources/glowdot.png"
            colorVariation: 0.1
            color: "#FF9999FF"
        }
        ImageParticle {
            groups: ["edge"]
            anchors.fill: parent
            source: "qrc:///particleresources/glowdot.png"
            colorVariation: 0.1
            color: "#000099FF"
        }

        Emitter {
            anchors.fill: parent
            group: "center"
            emitRate: 900
            lifeSpan: 200
            size: 20 * devicePixelRatio
            sizeVariation: 2
            endSize: 0
            //! [0]
            shape: EllipseShape {fill: false}
            velocity: TargetDirection {
                targetX: particles.width/2
                targetY: particles.height/2
                proportionalMagnitude: true
                magnitude: 0.5
            }
            //! [0]
        }

        Emitter {
            anchors.fill: parent
            group: "edge"
            startTime: 200
            emitRate: 2000
            lifeSpan: 20
            size: 28 * devicePixelRatio
            sizeVariation: 2 * devicePixelRatio
            endSize: 16 * devicePixelRatio
            shape: EllipseShape {fill: false}
            velocity: TargetDirection {
                targetX: particles.width/2
                targetY: particles.height/2
                proportionalMagnitude: true
                magnitude: 0.1
                magnitudeVariation: 0.1
            }
            acceleration: TargetDirection {
                targetX: particles.width
                targetY: particles.height
                targetVariation: 200
                proportionalMagnitude: true
                magnitude: 0.1
                magnitudeVariation: 0.1
            }
        }
    }
}
