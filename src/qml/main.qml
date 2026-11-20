/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import CachyOS.UI

CachyWindow {
    id: root

    title: root.hasSelection && root.currentMemo.title
        ? root.currentMemo.title + " — Notas de Voz"
        : "Notas de Voz (KVoiceMemos)"
    width: 960
    height: 640
    minimumWidth: 740
    minimumHeight: 480
    visible: true

    showTrafficLights: true
    showTitleBarDivider: true
    showWindowFrame: true
    sidebarVisible: true
    sidebarWidthExpanded: 280
    cornerRadius: CachyTheme.radiusWindow

    readonly property var controller: typeof VoiceMemos !== "undefined" ? VoiceMemos : null
    readonly property var currentMemo: controller ? controller.selectedMemo : ({})
    readonly property bool hasSelection: currentMemo && currentMemo.id !== undefined && currentMemo.id !== ""
    readonly property bool isCurrentPlaying: Boolean(controller && hasSelection && controller.playingMemoId === currentMemo.id)

    // =========================================================================
    // 1. GOLDEN GATE EDGE-TO-EDGE SIDEBAR (Recordings List + Bottom Record Dock)
    // =========================================================================
    sidebarHeaderActions: [
        Row {
            spacing: 6
            anchors.verticalCenter: parent.verticalCenter

            CachyToolbarButton {
                iconName: (root.controller && root.controller.recording) ? "media-playback-stop" : "add"
                tooltipText: (root.controller && root.controller.recording)
                    ? qsTr("Detener grabación")
                    : qsTr("Nueva grabación rápida")
                onClicked: {
                    if (!root.controller) return;
                    if (root.controller.recording) {
                        root.controller.stopRecording();
                    } else {
                        root.controller.startRecording();
                    }
                }
            }
        }
    ]

    sidebar: [
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 12
            spacing: 10

            MemoListSidebar {
                Layout.fillWidth: true
                Layout.fillHeight: true
                controller: root.controller
            }

            RecordingControlDrawer {
                controller: root.controller
            }
        }
    ]

    // =========================================================================
    // 2. GOLDEN GATE 44PX HEADER TOOLBAR
    // =========================================================================
    headerLeading: [
        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            CachySidebarToggleButton {
                targetWindow: root
                visible: true
            }

            CachyToolbarButton {
                visible: root.hasSelection
                iconName: root.isCurrentPlaying ? "pause" : "play"
                tooltipText: root.isCurrentPlaying ? qsTr("Detener reproducción") : qsTr("Reproducir grabación seleccionada")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.playMemo(root.currentMemo.id);
                    }
                }
            }
        }
    ]

    headerActions: [
        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            CachyToolbarButton {
                visible: root.hasSelection
                iconName: "volume"
                tooltipText: qsTr("Transcribir audio con Whisper.cpp local")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.transcribeMemo(root.currentMemo.id);
                    }
                }
            }

            CachyToolbarButton {
                visible: root.hasSelection
                iconName: "sparkles"
                tooltipText: qsTr("Generar resumen ejecutivo con IA (Kity / Ollama)")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.summarizeMemo(root.currentMemo.id);
                    }
                }
            }

            CachyToolbarButton {
                visible: root.hasSelection
                iconName: "file"
                tooltipText: qsTr("Copiar transcripción al portapapeles")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.copyText(root.currentMemo.transcript || "");
                    }
                }
            }

            CachyToolbarButton {
                visible: root.hasSelection
                iconName: "upload"
                tooltipText: qsTr("Exportar nota y resumen a la base de conocimiento RAG de Kity")
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.exportToKityKnowledge(root.currentMemo.id);
                    }
                }
            }
        }
    ]

    // =========================================================================
    // 3. RIGHT DETAIL PANE (Acoustic Waveform + Whisper Transcript + AI Summary)
    // =========================================================================
    contentData: [
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            PlaybackWaveformCard {
                controller: root.controller
                currentMemo: root.currentMemo
                hasSelection: root.hasSelection
                isCurrentPlaying: root.isCurrentPlaying
            }

            TranscriptCard {
                controller: root.controller
                currentMemo: root.currentMemo
                hasSelection: root.hasSelection
            }

            SummaryCard {
                controller: root.controller
                currentMemo: root.currentMemo
                hasSelection: root.hasSelection
            }

            EmptyMemoPlaceholder {
                visible: !root.hasSelection
            }

            // Bottom Status Bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 7
                    Layout.preferredHeight: 7
                    radius: 3.5
                    color: (root.controller && root.controller.recording) ? "#FF3B30" : "#34C759"
                }

                Text {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: (root.controller && root.controller.statusMessage)
                        ? root.controller.statusMessage
                        : "Listo • PipeWire + Whisper.cpp + Kity RAG"
                    font.pixelSize: 11
                    color: CachyTheme.textSecondary
                }
            }
        }
    ]
}
