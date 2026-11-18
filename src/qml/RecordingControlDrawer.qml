/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import CachyOS.UI

Rectangle {
    id: root

    required property var controller

    Layout.fillWidth: true
    implicitHeight: recDockCol.implicitHeight + 20
    radius: 16
    color: CachyTheme.cardBackground
    border.width: 1
    border.color: (root.controller && root.controller.recording)
        ? "#FF3B30"
        : CachyTheme.specularBorder

    ColumnLayout {
        id: recDockCol
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: (root.controller && root.controller.recording) ? "#FF3B30" : CachyTheme.textMuted
            }

            Text {
                text: root.controller
                    ? root.controller.formatDuration(root.controller.recordingSeconds)
                    : "00:00"
                font.pixelSize: 14
                font.weight: Font.Bold
                color: (root.controller && root.controller.recording) ? "#FF3B30" : CachyTheme.textPrimary
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 8
                radius: 4
                color: CachyTheme.surfacePill
                clip: true

                Rectangle {
                    width: parent.width * (root.controller ? Math.min(1.0, root.controller.currentLevel) : 0.0)
                    height: parent.height
                    radius: 4
                    color: "#FF3B30"
                    Behavior on width { NumberAnimation { duration: 100 } }
                }
            }
        }

        // Iconic Apple Voice Memos Red Pill Record Button
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            radius: 19
            color: (root.controller && root.controller.recording) ? "#D70015" : "#FF3B30"

            RowLayout {
                anchors.centerIn: parent
                spacing: 6

                Rectangle {
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    radius: (root.controller && root.controller.recording) ? 2 : 5
                    color: "#FFFFFF"
                }

                Text {
                    text: (root.controller && root.controller.recording)
                        ? "Detener y transcribir"
                        : "Grabar Nota de Voz"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    color: "#FFFFFF"
                }
            }

            TapHandler {
                onTapped: {
                    if (!root.controller) return;
                    if (root.controller.recording) {
                        root.controller.stopRecording();
                    } else {
                        root.controller.startRecording();
                    }
                }
            }
        }
    }
}
