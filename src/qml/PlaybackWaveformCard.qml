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
    required property bool isCurrentPlaying

    Layout.fillWidth: true
    visible: root.hasSelection
    implicitHeight: waveCol.implicitHeight + 24
    radius: 16
    color: CachyTheme.cardBackground
    border.width: 1
    border.color: CachyTheme.specularBorder

    ColumnLayout {
        id: waveCol
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            QQC2.TextField {
                id: titleEditor
                Layout.fillWidth: true
                text: root.currentMemo.title || ""
                placeholderText: "Título de la nota de voz…"
                font.pixelSize: 16
                font.bold: true
                color: CachyTheme.textPrimary
                background: null
                onAccepted: {
                    if (root.controller && root.hasSelection) {
                        root.controller.renameMemo(root.currentMemo.id, text);
                    }
                }
            }

            CachyToolbarButton {
                text: "Guardar"
                implicitWidth: 68
                tooltipText: "Guardar nuevo título de la nota"
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.renameMemo(root.currentMemo.id, titleEditor.text);
                    }
                }
            }
        }

        // Golden Gate Acoustic Waveform Strip
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            radius: 12
            color: CachyTheme.isDark ? "#121318" : "#F2F3F7"
            border.width: 1
            border.color: CachyTheme.specularBorder
            clip: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 3

                Repeater {
                    model: 42
                    Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        readonly property real wavePattern: 0.22 + 0.72 * Math.abs(Math.sin(index * 0.47 + (root.isCurrentPlaying ? 1.4 : 0.0)))
                        Layout.preferredHeight: Math.max(6, Math.round(36 * wavePattern))
                        Layout.alignment: Qt.AlignVCenter
                        radius: 1.5
                        color: root.isCurrentPlaying ? "#FF3B30" : CachyTheme.accentColor
                        opacity: root.isCurrentPlaying ? 0.95 : 0.65
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: root.hasSelection
                    ? ("Fecha: " + (root.currentMemo.createdAt || "")
                       + "   •   Duración: "
                       + (root.controller ? root.controller.formatDuration(root.currentMemo.durationSec || 0) : "00:00"))
                    : ""
                font.pixelSize: 11
                color: CachyTheme.textSecondary
                elide: Text.ElideRight
            }
        }
    }
}
