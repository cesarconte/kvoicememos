/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import CachyOS.UI

Rectangle {
    id: root

    required property var controller
    required property var currentMemo
    required property bool hasSelection

    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.leftMargin: 24
    Layout.rightMargin: 24
    Layout.bottomMargin: 8
    radius: CachyTheme.radiusMedium
    color: CachyPalette.cardBackground
    border.width: 1
    border.color: CachyPalette.specularBorder
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: CachyTheme.marginCard
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Transcripción (Whisper.cpp)")
                font.pixelSize: 13
                font.weight: Font.Bold
                color: CachyPalette.accentText
                elide: Text.ElideRight
            }

            CachyBusyIndicator {
                running: Boolean(root.currentMemo.transcribing)
                visible: running
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
            }

            CachyButton {
                text: qsTr("Copiar")
                iconName: "copy"
                pillStyle: true
                enabled: Boolean(root.currentMemo.transcript && root.currentMemo.transcript.length > 0)
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.copyText(root.currentMemo.transcript || "");
                    }
                }
            }

            CachyButton {
                text: qsTr("Transcribir")
                iconName: "message"
                primary: true
                pillStyle: true
                enabled: !root.currentMemo.transcribing
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.transcribeMemo(root.currentMemo.id);
                    }
                }
            }
        }

        Flickable {
            id: transcriptFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: transcriptTextEdit.implicitHeight + 12
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            QQC2.ScrollBar.vertical: QQC2.ScrollBar {
                policy: QQC2.ScrollBar.AsNeeded
            }

            TextEdit {
                id: transcriptTextEdit
                width: Math.max(100, transcriptFlick.width - 8)
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                text: root.currentMemo.transcript
                    ? root.currentMemo.transcript
                    : qsTr("Pulsa «Transcribir» para convertir la grabación en texto mediante Whisper.cpp local.")
                color: root.currentMemo.transcript ? CachyPalette.text : CachyPalette.textMuted
                selectionColor: CachyTheme.accentColor
                selectedTextColor: "#FFFFFF"
                font.pixelSize: 13
            }
        }
    }
}
