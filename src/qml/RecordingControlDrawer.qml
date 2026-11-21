/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import CachyOS.UI

// macOS 27 Golden Gate Voice Memos Bottom Control Dock:
// Left: [15s Back] [Play/Pause] [15s Forward]
// Center: Frosted-Glass Capsule with Red Record / Stop Control
// Right: Tabular Elapsed Timer (00:06.74)
Item {
    id: root

    required property var controller
    required property var currentMemo
    required property bool hasSelection
    required property bool isCurrentPlaying

    readonly property bool isRecording: Boolean(controller && controller.recording)
    readonly property bool playbackControlsEnabled: !isRecording && hasSelection

    Layout.fillWidth: true
    implicitHeight: 56

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 12

        // Left: Unboxed 15s Back / Play-Pause / 15s Forward
        Row {
            spacing: 14
            opacity: root.playbackControlsEnabled ? 1.0 : 0.34

            CachyToolbarButton {
                pillStyle: false
                iconName: "skip-back-15"
                enabled: root.playbackControlsEnabled
                tooltipText: qsTr("Retroceder 15 segundos")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.skipPlayback(root.currentMemo.id, -15.0);
                    }
                }
            }

            CachyToolbarButton {
                pillStyle: false
                iconName: root.isCurrentPlaying ? "pause" : "play"
                enabled: root.playbackControlsEnabled
                tooltipText: root.isCurrentPlaying ? qsTr("Pausar (Espacio)") : qsTr("Reproducir (Espacio)")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.togglePlayPause(root.currentMemo.id);
                    }
                }
            }

            CachyToolbarButton {
                pillStyle: false
                iconName: "skip-forward-15"
                enabled: root.playbackControlsEnabled
                tooltipText: qsTr("Avanzar 15 segundos")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.skipPlayback(root.currentMemo.id, 15.0);
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Center: Iconic Golden Gate Frosted-Glass Capsule Record / Stop Dock
        Rectangle {
            id: centerRecordCapsule
            Layout.preferredWidth: 112
            Layout.preferredHeight: 36
            radius: 18
            color: recordTap.pressed
                ? CachyPalette.pressedOverlay
                : (recordHover.hovered ? CachyPalette.cardBackgroundHover : CachyPalette.surfacePill)
            border.width: 1
            border.color: root.isRecording ? Qt.rgba(1.0, 0.23, 0.19, 0.45) : CachyPalette.specularBorder

            scale: recordTap.pressed ? 0.96 : (recordHover.hovered ? 1.02 : 1.0)
            Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutBack } }

            // While recording: iconic double red pause/stop bars (27-Golden-Gate-Voice-Memos.png)
            Row {
                anchors.centerIn: parent
                spacing: 4
                visible: root.isRecording

                Rectangle {
                    width: 4
                    height: 14
                    radius: 1.5
                    color: "#FF3B30"
                }
                Rectangle {
                    width: 4
                    height: 14
                    radius: 1.5
                    color: "#FF3B30"
                }
            }

            // While idle: crisp red record circle inside frosted glass capsule
            Rectangle {
                anchors.centerIn: parent
                visible: !root.isRecording
                width: 16
                height: 16
                radius: 8
                color: "#FF3B30"
            }

            HoverHandler {
                id: recordHover
                cursorShape: Qt.PointingHandCursor
            }
            TapHandler {
                id: recordTap
                onTapped: {
                    if (!root.controller) return;
                    if (root.isRecording) {
                        root.controller.stopRecording();
                    } else {
                        root.controller.startRecording();
                    }
                }
            }
            QQC2.ToolTip.visible: recordHover.hovered
            QQC2.ToolTip.text: root.isRecording
                ? qsTr("Detener y guardar grabación")
                : qsTr("Iniciar grabación de voz (Ctrl+N)")
            QQC2.ToolTip.delay: 450
        }

        Item { Layout.fillWidth: true }

        // Right: Bold Tabular Elapsed Timer (00:06.74 during recording)
        Text {
            text: {
                if (!root.controller) return "00:00.00";
                if (root.isRecording) {
                    return root.controller.formatElapsedPrecise(root.controller.recordingElapsedMs);
                }
                if (root.hasSelection && root.controller.playingMemoId === root.currentMemo.id) {
                    return root.controller.formatElapsedPrecise(Math.round(root.controller.playbackPositionSec * 1000));
                }
                if (root.hasSelection) {
                    return root.controller.formatDuration(root.currentMemo.durationSec || 0);
                }
                return "00:00";
            }
            font.pixelSize: 20
            font.weight: Font.Bold
            font.features: ({ "tnum": 1 })
            color: CachyPalette.text
            horizontalAlignment: Text.AlignRight
        }
    }
}
