/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
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
    sidebarVisible: root.showSidebar
    sidebarWidth: 280
    cornerRadius: CachyTheme.radiusWindow

    property bool showSidebar: true
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
                text: (root.controller && root.controller.recording) ? "■" : "●"
                tooltipText: (root.controller && root.controller.recording)
                    ? "Detener grabación"
                    : "Nueva grabación rápida"
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

            // Section Header
            Text {
                Layout.leftMargin: 6
                text: "All Recordings"
                font.pixelSize: 11
                font.weight: Font.DemiBold
                color: CachyTheme.textMuted
            }

            // Rounded Search Pill
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                radius: 16
                color: CachyTheme.surfacePill
                border.width: 1
                border.color: searchField.activeFocus ? CachyTheme.accentColor : CachyTheme.specularBorder

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 6

                    Text {
                        text: "🔍"
                        font.pixelSize: 11
                        color: CachyTheme.textMuted
                    }

                    QQC2.TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: "Buscar grabaciones…"
                        font.pixelSize: 12
                        background: null
                        color: CachyTheme.textPrimary
                        placeholderTextColor: CachyTheme.textMuted
                        onTextChanged: {
                            if (root.controller) {
                                root.controller.filterMemos(text);
                            }
                        }
                    }
                }
            }

            // Recordings ListView (Strictly Zero Horizontal Overflow)
            ListView {
                id: memoListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 6
                model: root.controller ? root.controller.memos : []

                delegate: Rectangle {
                    id: delegateRoot
                    required property var modelData
                    width: ListView.view ? ListView.view.width : 250
                    height: 68
                    radius: 12

                    readonly property bool isSelected: Boolean(root.controller
                        && root.controller.selectedMemoId === delegateRoot.modelData.id)
                    readonly property bool isPlaying: Boolean(root.controller
                        && root.controller.playingMemoId === delegateRoot.modelData.id)

                    color: isSelected
                        ? CachyTheme.withAlpha(CachyTheme.accentColor, CachyTheme.isDark ? 0.24 : 0.16)
                        : (rowHover.hovered ? CachyTheme.cardBackgroundHover : "transparent")
                    border.width: isSelected ? 1 : 0
                    border.color: isSelected ? CachyTheme.accentColor : "transparent"

                    HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler {
                        onTapped: {
                            if (root.controller) {
                                root.controller.selectMemo(delegateRoot.modelData.id);
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        // Play / Stop Circle Pill
                        Rectangle {
                            width: 30
                            height: 30
                            radius: 15
                            color: delegateRoot.isPlaying ? "#FF3B30" : CachyTheme.surfacePill
                            border.width: 1
                            border.color: CachyTheme.specularBorder

                            Text {
                                anchors.centerIn: parent
                                text: delegateRoot.isPlaying ? "■" : "▶"
                                font.pixelSize: 11
                                color: delegateRoot.isPlaying ? "#FFFFFF" : CachyTheme.accentColor
                            }

                            TapHandler {
                                onTapped: {
                                    if (root.controller) {
                                        root.controller.selectMemo(delegateRoot.modelData.id);
                                        root.controller.playMemo(delegateRoot.modelData.id);
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                Layout.fillWidth: true
                                text: delegateRoot.modelData.title || "Nota de voz"
                                font.pixelSize: 13
                                font.weight: delegateRoot.isSelected ? Font.Bold : Font.DemiBold
                                elide: Text.ElideRight
                                color: CachyTheme.textPrimary
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Text {
                                    Layout.fillWidth: true
                                    text: delegateRoot.modelData.createdAt || ""
                                    font.pixelSize: 11
                                    color: CachyTheme.textSecondary
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: root.controller
                                        ? root.controller.formatDuration(delegateRoot.modelData.durationSec || 0)
                                        : "00:00"
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    color: CachyTheme.textMuted
                                }
                            }
                        }

                        // Delete Button on Hover
                        Rectangle {
                            width: 24
                            height: 24
                            radius: 12
                            visible: rowHover.hovered || delegateRoot.isSelected
                            color: delHover.hovered ? "#28FF3B30" : "transparent"

                            HoverHandler { id: delHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onTapped: {
                                    if (root.controller) {
                                        root.controller.deleteMemo(delegateRoot.modelData.id);
                                    }
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "🗑"
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: memoListView.count === 0
                    width: parent.width - 24
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: "No hay grabaciones.\nPulsa el botón rojo inferior para grabar con PipeWire."
                    font.pixelSize: 12
                    color: CachyTheme.textMuted
                }
            }

            // Bottom Golden Gate Voice Memos Recording Dock
            Rectangle {
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
                            width: 8
                            height: 8
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
                                width: 10
                                height: 10
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
        }
    ]

    // =========================================================================
    // 2. GOLDEN GATE 44PX HEADER TOOLBAR
    // =========================================================================
    headerLeading: [
        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            CachyToolbarButton {
                text: "☰"
                tooltipText: "Mostrar/ocultar barra lateral de grabaciones"
                onClicked: root.showSidebar = !root.showSidebar
            }

            CachyToolbarButton {
                visible: root.hasSelection
                text: root.isCurrentPlaying ? "■" : "▶"
                tooltipText: root.isCurrentPlaying ? "Detener reproducción" : "Reproducir grabación seleccionada"
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
                text: "🗣"
                tooltipText: "Transcribir audio con Whisper.cpp local"
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.transcribeMemo(root.currentMemo.id);
                    }
                }
            }

            CachyToolbarButton {
                visible: root.hasSelection
                text: "✨"
                tooltipText: "Generar resumen ejecutivo con IA (Apple Intelligence / Ollama)"
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.summarizeMemo(root.currentMemo.id);
                    }
                }
            }

            CachyToolbarButton {
                visible: root.hasSelection
                text: "📋"
                tooltipText: "Copiar transcripción al portapapeles"
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.copyText(root.currentMemo.transcript || "");
                    }
                }
            }

            CachyToolbarButton {
                visible: root.hasSelection
                text: "✦"
                tooltipText: "Exportar nota y resumen a la base de conocimiento RAG de Kity"
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

            // A. Top Acoustic Waveform & Title Card
            Rectangle {
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

            // B. Whisper.cpp Local Transcript Card (Zero Horizontal Overflow)
            Rectangle {
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

            // C. Apple Intelligence / Kity Summary Card (Zero Horizontal Overflow)
            Rectangle {
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

            // D. Empty State When No Recording Is Selected
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !root.hasSelection
                radius: 16
                color: CachyTheme.cardBackground
                border.width: 1
                border.color: CachyTheme.specularBorder

                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(380, parent.width - 32)
                    spacing: 12

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 64
                        height: 64
                        radius: 32
                        color: "#1FFF3B30"
                        border.width: 1
                        border.color: "#4DFF3B30"

                        Text {
                            anchors.centerIn: parent
                            text: "🎙"
                            font.pixelSize: 28
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: "Selecciona o graba una nota de voz"
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: CachyTheme.textPrimary
                    }

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: "Las grabaciones se capturan con PipeWire, se transcriben localmente con Whisper.cpp y pueden resumirse o enviarse a la base de conocimiento RAG de Kity."
                        font.pixelSize: 12
                        color: CachyTheme.textSecondary
                    }
                }
            }

            // Bottom Status Bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    width: 7
                    height: 7
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
