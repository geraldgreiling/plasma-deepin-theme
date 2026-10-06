/*
    SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
    SPDX-License-Identifier: GPL-3.0-or-later

    Deepin style splash screen: calm gradient, launcher icon of the icon
    theme in the centre, three pulsing dots below (like DTK's spinner).
    Uses only QtQuick and Kirigami, no private Plasma API.
*/
import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    id: root

    // set by ksplashqml: 1 = initial, ..., 5 = done
    property int stage

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#1c2433" }
        GradientStop { position: 1.0; color: "#0d1117" }
    }

    onStageChanged: {
        if (stage === 2) {
            fadeIn.running = true;
        } else if (stage === 5) {
            fadeOut.running = true;
        }
    }

    Item {
        id: content
        anchors.fill: parent
        opacity: 0

        Kirigami.Icon {
            id: logo
            readonly property real size: Kirigami.Units.gridUnit * 6
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -Kirigami.Units.gridUnit * 2
            width: size
            height: size
            source: "start-here-kde-plasma"
            fallback: "start-here"
        }

        Row {
            id: dots
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: logo.bottom
            anchors.topMargin: Kirigami.Units.gridUnit * 3
            spacing: Kirigami.Units.gridUnit * 0.6

            Repeater {
                model: 3
                Rectangle {
                    id: dot
                    required property int index
                    width: Kirigami.Units.gridUnit * 0.55
                    height: width
                    radius: width / 2
                    color: "#0081ff"
                    opacity: 0.3

                    SequentialAnimation on opacity {
                        running: Kirigami.Units.longDuration > 1
                        loops: Animation.Infinite
                        PauseAnimation { duration: dot.index * 200 }
                        NumberAnimation { to: 1.0; duration: 400; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 0.3; duration: 400; easing.type: Easing.InOutQuad }
                        PauseAnimation { duration: (2 - dot.index) * 200 }
                    }
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Kirigami.Units.gridUnit * 2
            color: "#8b95a7"
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            text: "Plasma"
            textFormat: Text.PlainText
        }
    }

    OpacityAnimator {
        id: fadeIn
        target: content
        from: 0
        to: 1
        duration: Kirigami.Units.veryLongDuration * 2
        easing.type: Easing.InOutQuad
    }

    OpacityAnimator {
        id: fadeOut
        target: content
        from: 1
        to: 0
        duration: Kirigami.Units.veryLongDuration
        easing.type: Easing.InOutQuad
    }
}
