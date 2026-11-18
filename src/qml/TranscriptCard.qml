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
    visible: root.hasSelection
    radius: 16
    color: CachyTheme.cardBackground
    border.width: 1
    border.color: CachyTheme.specularBorder
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: "Transcripción (Whisper.cpp)"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: CachyTheme.accentColor
                elide: Text.ElideRight
            }

            QQC2.BusyIndicator {
                running: Boolean(root.currentMemo.transcribing)
                visible: running
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
            }

            CachyToolbarButton {
                text: "Transcribir"
                implicitWidth: 82
                enabled: !root.currentMemo.transcribing
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.transcribeMemo(root.currentMemo.id);
                    }
                }
            }
        }

        QQC2.ScrollView {
            id: transcriptScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            clip: true

            QQC2.TextArea {
                width: transcriptScroll.availableWidth
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                text: root.currentMemo.transcript
                    ? root.currentMemo.transcript
                    : "Pulsa «Transcribir» para convertir la grabación en texto mediante Whisper.cpp local."
                color: root.currentMemo.transcript ? CachyTheme.textPrimary : CachyTheme.textMuted
                font.pixelSize: 13
                background: null
            }
        }
    }
}
