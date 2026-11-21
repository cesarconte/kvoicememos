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
                text: qsTr("Resumen IA (Kity / Ollama)")
                font.pixelSize: 13
                font.weight: Font.Bold
                color: "#AF52DE"
                elide: Text.ElideRight
            }

            CachyBusyIndicator {
                running: Boolean(root.currentMemo.summarizing)
                visible: running
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
            }

            CachyButton {
                text: qsTr("Copiar")
                iconName: "copy"
                pillStyle: true
                enabled: Boolean(root.currentMemo.summary && root.currentMemo.summary.length > 0)
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.copyText(root.currentMemo.summary || "");
                    }
                }
            }

            CachyButton {
                text: qsTr("Resumir")
                iconName: "sparkles"
                primary: true
                pillStyle: true
                enabled: !root.currentMemo.summarizing && Boolean(root.currentMemo.transcript)
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.summarizeMemo(root.currentMemo.id);
                    }
                }
            }

            CachyButton {
                text: qsTr("Kity RAG")
                iconName: "upload"
                pillStyle: true
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.exportToKityKnowledge(root.currentMemo.id);
                    }
                }
            }
        }

        Flickable {
            id: summaryFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: summaryTextEdit.implicitHeight + 12
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            QQC2.ScrollBar.vertical: QQC2.ScrollBar {
                policy: QQC2.ScrollBar.AsNeeded
            }

            TextEdit {
                id: summaryTextEdit
                width: Math.max(100, summaryFlick.width - 8)
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                text: root.currentMemo.summary
                    ? root.currentMemo.summary
                    : qsTr("El resumen ejecutivo con puntos clave aparecerá aquí.")
                color: root.currentMemo.summary ? CachyPalette.text : CachyPalette.textMuted
                selectionColor: "#AF52DE"
                selectedTextColor: "#FFFFFF"
                font.pixelSize: 13
            }
        }
    }
}
