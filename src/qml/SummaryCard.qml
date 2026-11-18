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
                text: "Resumen IA (Kity / Ollama)"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: "#AF52DE"
                elide: Text.ElideRight
            }

            QQC2.BusyIndicator {
                running: Boolean(root.currentMemo.summarizing)
                visible: running
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
            }

            CachyToolbarButton {
                text: "Resumir"
                implicitWidth: 70
                enabled: !root.currentMemo.summarizing && Boolean(root.currentMemo.transcript)
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.summarizeMemo(root.currentMemo.id);
                    }
                }
            }

            CachyToolbarButton {
                text: "✦ Kity RAG"
                implicitWidth: 84
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.exportToKityKnowledge(root.currentMemo.id);
                    }
                }
            }
        }

        QQC2.ScrollView {
            id: summaryScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            clip: true

            QQC2.TextArea {
                width: summaryScroll.availableWidth
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                text: root.currentMemo.summary
                    ? root.currentMemo.summary
                    : "El resumen ejecutivo con puntos clave aparecerá aquí."
                color: root.currentMemo.summary ? CachyTheme.textPrimary : CachyTheme.textMuted
                font.pixelSize: 13
                background: null
            }
        }
    }
}
