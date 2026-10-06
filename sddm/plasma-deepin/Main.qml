/*
    SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
    SPDX-License-Identifier: GPL-3.0-or-later

    SDDM login theme in the style of the Deepin desktop (Qt 6).
    Blurred background, clock at the top, avatar with a frosted password
    field in the centre, session and power buttons at the bottom right.
    Uses only QtQuick, QtQuick.Controls.Basic and QtQuick.Effects.
*/
import QtQuick
import QtQuick.Controls.Basic as QQC
import QtQuick.Effects

Rectangle {
    id: root
    width: 1920
    height: 1080
    color: "#10151f"

    readonly property color accent: (typeof config !== "undefined" && config.accentColor) ? config.accentColor : "#0081ff"
    readonly property string backgroundSource: (typeof config !== "undefined" && config.background) ? config.background : ""
    readonly property int blurRadius: (typeof config !== "undefined" && config.blur !== undefined && config.blur !== "") ? Number(config.blur) : 64
    readonly property bool use24h: (typeof config === "undefined" || config.use24h === undefined) ? true : String(config.use24h) !== "false"
    readonly property real unit: Math.max(8, Math.round(height / 90))

    property int userIndex: userModel.lastIndex >= 0 ? userModel.lastIndex : 0
    property int sessionIndex: sessionModel.lastIndex >= 0 ? sessionModel.lastIndex : 0
    property string userName: userModel.lastUser
    property string realName: userName
    property string avatar: ""

    TextMetrics { id: metrics }

    // ------------------------------------------------------------------ background
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#25406b" }
            GradientStop { position: 0.55; color: "#162238" }
            GradientStop { position: 1.0; color: "#0b0f17" }
        }
    }
    Image {
        id: wallpaper
        anchors.fill: parent
        source: root.backgroundSource
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        visible: false
    }
    MultiEffect {
        anchors.fill: parent
        source: wallpaper
        visible: wallpaper.status === Image.Ready
        blurEnabled: root.blurRadius > 0
        blurMax: root.blurRadius
        blur: 1.0
        brightness: -0.08
    }
    Rectangle { anchors.fill: parent; color: "#000000"; opacity: 0.18 }

    // ------------------------------------------------------------------ clock
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: root.height * 0.12
        spacing: root.unit * 0.5

        Text {
            id: clock
            anchors.horizontalCenter: parent.horizontalCenter
            color: "white"
            font.pixelSize: root.unit * 7
            font.weight: Font.Light
            text: Qt.formatTime(new Date(), root.use24h ? "hh:mm" : "h:mm AP")
        }
        Text {
            id: date
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#e6ffffff"
            font.pixelSize: root.unit * 1.8
            text: Qt.formatDate(new Date(), Qt.locale().dateFormat(Locale.LongFormat))
        }
        Timer {
            interval: 1000
            running: true
            repeat: true
            onTriggered: {
                clock.text = Qt.formatTime(new Date(), root.use24h ? "hh:mm" : "h:mm AP");
                date.text = Qt.formatDate(new Date(), Qt.locale().dateFormat(Locale.LongFormat));
            }
        }
    }

    // ------------------------------------------------------------------ login box
    Column {
        id: loginBox
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.unit * 4
        spacing: root.unit * 1.6

        // avatar
        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.unit * 12
            height: width

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "#33ffffff"
                border.color: "#66ffffff"
                border.width: 2
            }
            Image {
                id: avatarImage
                anchors.fill: parent
                anchors.margins: 4
                source: root.avatar
                fillMode: Image.PreserveAspectCrop
                visible: false
                asynchronous: true
            }
            Rectangle {
                id: avatarMask
                anchors.fill: avatarImage
                radius: width / 2
                visible: false
                layer.enabled: true
            }
            MultiEffect {
                anchors.fill: avatarImage
                source: avatarImage
                maskEnabled: true
                maskSource: avatarMask
                visible: avatarImage.status === Image.Ready
            }
            Text {
                anchors.centerIn: parent
                visible: avatarImage.status !== Image.Ready
                color: "white"
                font.pixelSize: root.unit * 5
                text: (root.realName || root.userName || "?").charAt(0).toUpperCase()
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            color: "white"
            font.pixelSize: root.unit * 2.2
            text: root.realName || root.userName
        }

        // frosted password field with the login arrow inside
        Rectangle {
            id: passwordFrame
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.unit * 28
            height: root.unit * 4.4
            radius: root.unit * 1.0
            color: "#40ffffff"
            border.color: password.activeFocus ? root.accent : "#59ffffff"
            border.width: password.activeFocus ? 2 : 1

            QQC.TextField {
                id: password
                anchors.left: parent.left
                anchors.right: loginButton.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: root.unit
                echoMode: TextInput.Password
                passwordCharacter: "•"
                placeholderText: qsTr("Password")
                placeholderTextColor: "#ccffffff"
                color: "white"
                font.pixelSize: root.unit * 1.6
                focus: true
                background: Item {}
                onAccepted: root.doLogin()
                Keys.onEscapePressed: text = ""
            }
            Rectangle {
                id: loginButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: root.unit * 0.5
                width: parent.height - root.unit
                height: width
                radius: root.unit * 0.8
                color: loginArea.pressed ? Qt.darker(root.accent, 1.2) : (loginArea.containsMouse ? Qt.lighter(root.accent, 1.1) : root.accent)
                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.strokeStyle = "white";
                        ctx.lineWidth = 2;
                        ctx.lineCap = "round";
                        ctx.lineJoin = "round";
                        var w = width, h = height;
                        ctx.beginPath();
                        ctx.moveTo(w * 0.3, h * 0.5); ctx.lineTo(w * 0.7, h * 0.5);
                        ctx.moveTo(w * 0.52, h * 0.32); ctx.lineTo(w * 0.7, h * 0.5); ctx.lineTo(w * 0.52, h * 0.68);
                        ctx.stroke();
                    }
                }
                MouseArea {
                    id: loginArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.doLogin()
                }
            }
        }

        Text {
            id: message
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#ffb3a6"
            font.pixelSize: root.unit * 1.4
            text: (typeof keyboard !== "undefined" && keyboard && keyboard.capsLock) ? qsTr("Caps Lock is on") : ""
        }

        // other users
        ListView {
            id: userList
            anchors.horizontalCenter: parent.horizontalCenter
            visible: count > 1
            width: Math.min(contentWidth, root.width * 0.6)
            height: root.unit * 5
            orientation: ListView.Horizontal
            spacing: root.unit
            model: userModel
            interactive: contentWidth > width
            delegate: Rectangle {
                required property int index
                required property string name
                required property string realName
                required property string icon
                width: root.unit * 5
                height: width
                radius: width / 2
                color: index === root.userIndex ? "#66ffffff" : "#26ffffff"
                border.color: index === root.userIndex ? root.accent : "transparent"
                border.width: 2
                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pixelSize: root.unit * 2
                    text: (realName || name).charAt(0).toUpperCase()
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.selectUser(index, name, realName, icon)
                }
            }
        }
    }

    // picks the preselected user once the model is available
    Repeater {
        model: userModel
        delegate: Item {
            required property int index
            required property string name
            required property string realName
            required property string icon
            Component.onCompleted: if (index === root.userIndex) root.selectUser(index, name, realName, icon)
        }
    }

    // ------------------------------------------------------------------ bottom bar
    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: root.unit * 3
        spacing: root.unit * 1.2

        QQC.ComboBox {
            id: sessionBox
            width: root.unit * 18
            height: root.unit * 4.4
            model: sessionModel
            textRole: "name"
            currentIndex: root.sessionIndex
            onActivated: function(i) { root.sessionIndex = i; }
            font.pixelSize: root.unit * 1.4
            background: Rectangle {
                radius: root.unit
                color: sessionBox.hovered ? "#4dffffff" : "#33ffffff"
                border.color: "#40ffffff"
            }
            contentItem: Text {
                leftPadding: root.unit
                verticalAlignment: Text.AlignVCenter
                color: "white"
                font: sessionBox.font
                elide: Text.ElideRight
                text: sessionBox.displayText
            }
        }

        Repeater {
            model: [
                { kind: "suspend", label: qsTr("Suspend"), enabled: sddm.canSuspend },
                { kind: "reboot", label: qsTr("Restart"), enabled: sddm.canReboot },
                { kind: "poweroff", label: qsTr("Shut down"), enabled: sddm.canPowerOff }
            ]
            delegate: Rectangle {
                required property var modelData
                visible: modelData.enabled
                width: root.unit * 4.4
                height: width
                radius: width / 2
                color: area.pressed ? "#66ffffff" : (area.containsMouse ? "#4dffffff" : "#33ffffff")
                border.color: "#40ffffff"
                QQC.ToolTip.visible: area.containsMouse
                QQC.ToolTip.text: modelData.label
                Canvas {
                    anchors.fill: parent
                    property string kind: parent.modelData.kind
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.strokeStyle = "white";
                        ctx.fillStyle = "white";
                        ctx.lineWidth = 2;
                        ctx.lineCap = "round";
                        var cx = width / 2, cy = height / 2, r = width * 0.2;
                        ctx.beginPath();
                        if (kind === "poweroff") {
                            ctx.arc(cx, cy, r, -Math.PI / 2 + 0.6, 3 * Math.PI / 2 - 0.6, false);
                            ctx.moveTo(cx, cy - r * 1.25); ctx.lineTo(cx, cy - r * 0.2);
                        } else if (kind === "reboot") {
                            ctx.arc(cx, cy, r, -Math.PI / 2, Math.PI * 1.25, false);
                            ctx.moveTo(cx, cy - r); ctx.lineTo(cx + r * 0.55, cy - r * 1.45);
                            ctx.moveTo(cx, cy - r); ctx.lineTo(cx + r * 0.55, cy - r * 0.55);
                        } else {
                            // crescent moon: a disc with an offset disc cut out
                            ctx.arc(cx, cy, r * 1.05, 0, 2 * Math.PI);
                            ctx.fill();
                            ctx.globalCompositeOperation = "destination-out";
                            ctx.beginPath();
                            ctx.arc(cx + r * 0.55, cy - r * 0.45, r * 0.85, 0, 2 * Math.PI);
                            ctx.fill();
                            return;
                        }
                        ctx.stroke();
                    }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (parent.modelData.kind === "poweroff") sddm.powerOff();
                        else if (parent.modelData.kind === "reboot") sddm.reboot();
                        else sddm.suspend();
                    }
                }
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: root.unit * 3
        color: "#b3ffffff"
        font.pixelSize: root.unit * 1.3
        text: sddm.hostName
    }

    // ------------------------------------------------------------------ logic
    function selectUser(index, name, realName, icon) {
        root.userIndex = index;
        root.userName = name;
        root.realName = realName ? realName : name;
        root.avatar = icon ? icon : "";
        password.text = "";
        password.forceActiveFocus();
    }

    function doLogin() {
        message.text = "";
        sddm.login(root.userName, password.text, root.sessionIndex);
    }

    Connections {
        target: sddm
        function onLoginFailed() {
            password.text = "";
            message.text = qsTr("Wrong password, please try again");
            shake.start();
        }
    }

    SequentialAnimation {
        id: shake
        NumberAnimation { target: passwordFrame; property: "anchors.horizontalCenterOffset"; to: -10; duration: 50 }
        NumberAnimation { target: passwordFrame; property: "anchors.horizontalCenterOffset"; to: 10; duration: 80 }
        NumberAnimation { target: passwordFrame; property: "anchors.horizontalCenterOffset"; to: -6; duration: 70 }
        NumberAnimation { target: passwordFrame; property: "anchors.horizontalCenterOffset"; to: 0; duration: 60 }
    }

    Component.onCompleted: password.forceActiveFocus()
}
