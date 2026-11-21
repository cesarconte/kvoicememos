/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as QtDialogs
import QtQuick.Layouts
import QtQuick.Window
import CachyOS.UI

CachyWindow {
    id: root

    title: root.hasSelection && root.currentMemo.title
        ? root.currentMemo.title + " — Notas de Voz"
        : qsTr("Notas de Voz (KVoiceMemos)")
    width: 1060
    height: 660
    minimumWidth: 780
    minimumHeight: 500
    visible: true

    showTrafficLights: true
    showTitle: false
    showTitleBarDivider: false
    showContentTitleBar: false
    showWindowFrame: true
    sidebarVisible: true
    sidebarWidthExpanded: 244
    cornerRadius: CachyTheme.radiusWindow
    contentBackground: CachyPalette.isDark
        ? Qt.rgba(0.11, 0.11, 0.13, 0.94)
        : Qt.rgba(1.0, 1.0, 1.0, 0.96)

    readonly property var controller: typeof VoiceMemos !== "undefined" ? VoiceMemos : null
    readonly property var currentMemo: controller ? controller.selectedMemo : ({})
    readonly property bool hasSelection: Boolean(currentMemo && currentMemo.id !== undefined && currentMemo.id !== "")
    readonly property bool isRecording: Boolean(controller && controller.recording)
    readonly property bool isCurrentPlaying: Boolean(controller && hasSelection && controller.playingMemoId === currentMemo.id && !controller.playbackPaused)
    readonly property bool isTrashMemo: Boolean(hasSelection && currentMemo.deleted)

    property bool listPaneVisible: true
    property string activePanel: "waveform" // "waveform" | "transcript" | "summary"
    property bool trimMode: false
    property string pendingExportMemoId: ""

    onIsRecordingChanged: {
        if (isRecording) {
            trimMode = false;
            activePanel = "waveform";
        }
    }

    onHasSelectionChanged: {
        if (!hasSelection) {
            trimMode = false;
            activePanel = "waveform";
        }
    }

    // Standard macOS Voice Memos Shortcuts
    Shortcut {
        sequences: [StandardKey.New]
        onActivated: {
            if (!root.controller) return;
            if (root.isRecording) {
                root.controller.stopRecording();
            } else {
                root.controller.startRecording();
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+R"
        onActivated: {
            if (!root.controller) return;
            if (root.isRecording) {
                root.controller.stopRecording();
            } else {
                root.controller.startRecording();
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+T"
        onActivated: {
            if (!root.isRecording && root.hasSelection) {
                root.activePanel = (root.activePanel === "transcript") ? "waveform" : "transcript";
            }
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (root.trimMode) {
                root.trimMode = false;
            }
        }
    }

    QtDialogs.FileDialog {
        id: importWavDialog
        title: qsTr("Importar archivo de audio WAV…")
        fileMode: QtDialogs.FileDialog.OpenFile
        nameFilters: [qsTr("Audio WAV (*.wav)"), qsTr("Todos los archivos (*)")]
        onAccepted: {
            if (root.controller) {
                root.controller.importWavFile(selectedFile.toString());
            }
        }
    }

    QtDialogs.FileDialog {
        id: exportWavDialog
        title: qsTr("Exportar copia de audio WAV…")
        fileMode: QtDialogs.FileDialog.SaveFile
        defaultSuffix: "wav"
        nameFilters: [qsTr("Audio WAV (*.wav)")]
        onAccepted: {
            if (root.controller && root.pendingExportMemoId.length > 0) {
                root.controller.exportAudioCopy(root.pendingExportMemoId, selectedFile.toString());
            }
        }
    }

    // =========================================================================
    // 1. GOLDEN GATE SIDEBAR HEADER ACTIONS (New Folder + Toggle Sidebar)
    // =========================================================================
    sidebarHeaderActions: [
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            CachyToolbarButton {
                iconName: "folder-plus"
                tooltipText: qsTr("Nueva carpeta")
                onClicked: folderSidebar.startCreateFolder()
            }

            CachySidebarToggleButton {
                targetWindow: root
            }
        }
    ]

    // =========================================================================
    // 2. COLUMN 1 — EDGE-TO-EDGE TRANSLUCENT FOLDER SIDEBAR
    // =========================================================================
    sidebar: [
        FolderSidebar {
            id: folderSidebar
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            anchors.topMargin: 4
            anchors.bottomMargin: 12
            controller: root.controller
        }
    ]

    // =========================================================================
    // 3. COLUMN 2 + COLUMN 3 — GOLDEN GATE SPLIT VIEW (List + Waveform/AI)
    // =========================================================================
    contentData: [
        CachySplitView {
            anchors.fill: parent
            orientation: Qt.Horizontal

            // COLUMN 2 — RECORDINGS LIST PANE
            MemoListSidebar {
                visible: root.listPaneVisible
                QQC2.SplitView.preferredWidth: 264
                QQC2.SplitView.minimumWidth: 220
                QQC2.SplitView.maximumWidth: 340
                controller: root.controller
                targetWindow: root
                onImportAudioRequested: importWavDialog.open()
                onExportAudioRequested: memoId => {
                    root.pendingExportMemoId = memoId;
                    exportWavDialog.open();
                }
            }

            // COLUMN 3 — GOLDEN GATE MAIN WAVEFORM / TRANSCRIPT / AI CANVAS
            Item {
                QQC2.SplitView.fillWidth: true
                QQC2.SplitView.minimumWidth: 420

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // 44px Floating Header Toolbar (Exact match to 27-Golden-Gate-Voice-Memos.png)
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: CachyTheme.titleBarHeight
                        z: 10

                        DragHandler {
                            target: null
                            cursorShape: Qt.ClosedHandCursor
                            onActiveChanged: if (active) root.startSystemMove()
                        }
                        TapHandler {
                            enabled: root.maximizable
                            onDoubleTapped: {
                                if (root.visibility === Window.Maximized) {
                                    root.showNormal();
                                } else {
                                    root.showMaximized();
                                }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: (!root.sidebarVisible && !root.listPaneVisible) ? 16 : 14
                            anchors.rightMargin: 18
                            spacing: 8

                            CachyTrafficLights {
                                visible: !root.sidebarVisible && !root.listPaneVisible
                                window: root
                            }

                            CachySidebarToggleButton {
                                visible: !root.sidebarVisible && !root.listPaneVisible
                                targetWindow: root
                            }

                            CachyToolbarButton {
                                iconName: "list"
                                checkable: true
                                checked: root.listPaneVisible
                                tooltipText: root.listPaneVisible
                                    ? qsTr("Ocultar lista de grabaciones (modo enfoque)")
                                    : qsTr("Mostrar lista de grabaciones")
                                onToggled: nextState => root.listPaneVisible = nextState
                            }

                            Item { Layout.fillWidth: true }

                            // 3-Icon Playback / Transcript / Trim Capsule (27-Golden-Gate-Voice-Memos.png)
                            CachyToolbarGroup {
                                CachyToolbarButton {
                                    id: playbackOptsBtn
                                    objectName: "playbackOptsBtn"
                                    pillStyle: false
                                    iconName: "sliders"
                                    enabled: !root.isRecording
                                    tooltipText: qsTr("Ajustes de reproducción y mejora de audio")
                                    onClicked: playbackPopover.openAt(playbackOptsBtn)
                                }

                                CachyToolbarButton {
                                    objectName: "transcriptBtn"
                                    pillStyle: false
                                    iconName: "message"
                                    checkable: true
                                    checked: root.activePanel === "transcript"
                                    enabled: root.hasSelection && !root.isRecording
                                    tooltipText: root.hasSelection
                                        ? qsTr("Mostrar / ocultar transcripción (Ctrl+T)")
                                        : qsTr("Graba o selecciona una nota de voz para ver la transcripción")
                                    onToggled: nextState => root.activePanel = nextState ? "transcript" : "waveform"
                                }

                                CachyToolbarButton {
                                    objectName: "trimBtn"
                                    pillStyle: false
                                    iconName: "crop"
                                    checkable: true
                                    checked: root.trimMode
                                    accentColor: "#F0B817"
                                    enabled: root.hasSelection && !root.isRecording && !root.isTrashMemo
                                    tooltipText: root.hasSelection
                                        ? qsTr("Recortar grabación")
                                        : qsTr("Graba o selecciona una nota de voz para recortar")
                                    onToggled: nextState => {
                                        root.trimMode = nextState;
                                        if (nextState) {
                                            root.activePanel = "waveform";
                                        }
                                    }
                                }
                            }

                            // Standalone Blue Circular Done [✓] Button (27-Golden-Gate-Voice-Memos.png)
                            CachyToolbarButton {
                                visible: root.isRecording || root.trimMode
                                primary: true
                                iconName: "check"
                                tooltipText: root.isRecording
                                    ? qsTr("Finalizar y guardar grabación")
                                    : qsTr("Aplicar recorte")
                                onClicked: {
                                    if (!root.controller) return;
                                    if (root.isRecording) {
                                        root.controller.stopRecording();
                                    } else if (root.trimMode && root.hasSelection) {
                                        waveformCard.applyCurrentTrim();
                                    }
                                }
                            }

                            // 3-Icon AI Summary / Share / More Capsule (when viewing saved memo)
                            CachyToolbarGroup {
                                visible: !root.isRecording && !root.trimMode

                                CachyToolbarButton {
                                    objectName: "summaryBtn"
                                    pillStyle: false
                                    iconName: "sparkles"
                                    checkable: true
                                    checked: root.activePanel === "summary"
                                    accentColor: "#AF52DE"
                                    enabled: root.hasSelection
                                    tooltipText: root.hasSelection
                                        ? qsTr("Resumen IA (Kity / Ollama)")
                                        : qsTr("Graba o selecciona una nota de voz para ver el resumen IA")
                                    onToggled: nextState => root.activePanel = nextState ? "summary" : "waveform"
                                }

                                CachyToolbarButton {
                                    id: shareBtn
                                    objectName: "shareBtn"
                                    pillStyle: false
                                    iconName: "share"
                                    enabled: root.hasSelection
                                    tooltipText: root.hasSelection
                                        ? qsTr("Compartir y exportar a Kity RAG")
                                        : qsTr("Graba o selecciona una nota de voz para compartir o exportar")
                                    onClicked: sharePopover.openAt(shareBtn)
                                }

                                CachyToolbarButton {
                                    id: moreBtn
                                    objectName: "moreBtn"
                                    pillStyle: false
                                    iconName: "more-horizontal"
                                    enabled: true
                                    tooltipText: qsTr("Más acciones")
                                    onClicked: memoMoreMenu.popup(moreBtn, 0, moreBtn.height + 6)
                                }
                            }
                        }
                    }

                    // Trash Recovery Banner when viewing deleted memo
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 24
                        Layout.rightMargin: 24
                        Layout.bottomMargin: 8
                        Layout.preferredHeight: 38
                        radius: 10
                        visible: root.isTrashMemo && !root.isRecording
                        color: CachyPalette.cardBackground
                        border.width: 1
                        border.color: CachyPalette.specularBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 10
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: qsTr("Esta grabación está en Eliminadas")
                                font.pixelSize: 12
                                color: CachyPalette.textSecondary
                                elide: Text.ElideRight
                            }

                            CachyButton {
                                text: qsTr("Restaurar")
                                primary: true
                                pillStyle: true
                                onClicked: if (root.controller) root.controller.restoreMemo(root.currentMemo.id)
                            }

                            CachyButton {
                                text: qsTr("Eliminar")
                                destructive: true
                                pillStyle: true
                                onClicked: if (root.controller) root.controller.deleteMemo(root.currentMemo.id)
                            }
                        }
                    }

                    // Main Acoustic Waveform + Transcript / AI Summary Panels
                    PlaybackWaveformCard {
                        id: waveformCard
                        visible: root.isRecording || root.hasSelection
                        controller: root.controller
                        currentMemo: root.currentMemo
                        hasSelection: root.hasSelection
                        isCurrentPlaying: root.isCurrentPlaying
                        compactMode: !root.isRecording && root.activePanel !== "waveform"
                        trimMode: root.trimMode
                        onCancelTrimRequested: root.trimMode = false
                        onTrimDone: root.trimMode = false

                        function applyCurrentTrim() {
                            if (root.controller && root.hasSelection) {
                                const s0 = trimStartRatio * totalSeconds;
                                const s1 = trimEndRatio * totalSeconds;
                                if (root.controller.trimMemo(root.currentMemo.id, s0, s1, false)) {
                                    root.trimMode = false;
                                }
                            }
                        }
                    }

                    TranscriptCard {
                        visible: !root.isRecording && root.hasSelection && root.activePanel === "transcript"
                        controller: root.controller
                        currentMemo: root.currentMemo
                        hasSelection: root.hasSelection
                    }

                    SummaryCard {
                        visible: !root.isRecording && root.hasSelection && root.activePanel === "summary"
                        controller: root.controller
                        currentMemo: root.currentMemo
                        hasSelection: root.hasSelection
                    }

                    EmptyMemoPlaceholder {
                        visible: !root.isRecording && !root.hasSelection
                        onStartRecordingRequested: if (root.controller) root.controller.startRecording()
                        onImportAudioRequested: importWavDialog.open()
                    }

                    // Bottom Control Dock (27-Golden-Gate-Voice-Memos.png)
                    RecordingControlDrawer {
                        controller: root.controller
                        currentMemo: root.currentMemo
                        hasSelection: root.hasSelection
                        isCurrentPlaying: root.isCurrentPlaying
                    }

                    // Subtle Status Strip
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 24
                        Layout.rightMargin: 24
                        Layout.bottomMargin: 8
                        spacing: 7

                        Rectangle {
                            Layout.preferredWidth: 7
                            Layout.preferredHeight: 7
                            radius: 3.5
                            color: root.isRecording ? "#FF3B30" : "#34C759"
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                            text: (root.controller && root.controller.statusMessage)
                                ? root.controller.statusMessage
                                : qsTr("Listo · PipeWire + Whisper.cpp + Kity RAG")
                            font.pixelSize: 11
                            color: CachyPalette.textSecondary
                        }
                    }
                }
            }
        }
    ]

    // =========================================================================
    // 4. CONTEXT & MORE ACTIONS MENU
    // =========================================================================
    CachyMenu {
        id: memoMoreMenu
        objectName: "memoMoreMenu"

        CachyMenuItem {
            text: root.isRecording ? qsTr("Detener grabación") : qsTr("Nueva grabación")
            iconName: "microphone"
            onTriggered: {
                if (!root.controller) return;
                if (root.isRecording) {
                    root.controller.stopRecording();
                } else {
                    root.controller.startRecording();
                }
            }
        }
        CachyMenuItem {
            text: qsTr("Importar archivo WAV…")
            iconName: "upload"
            onTriggered: importWavDialog.open()
        }
        CachyMenuSeparator {}
        CachyMenuItem {
            text: qsTr("Transcribir con Whisper.cpp")
            iconName: "message"
            enabled: root.hasSelection && !root.isRecording
            onTriggered: {
                if (root.controller && root.hasSelection) {
                    root.activePanel = "transcript";
                    root.controller.transcribeMemo(root.currentMemo.id);
                }
            }
        }
        CachyMenuItem {
            text: qsTr("Resumir con IA (Ollama)")
            iconName: "sparkles"
            enabled: root.hasSelection && !root.isRecording
            onTriggered: {
                if (root.controller && root.hasSelection) {
                    root.activePanel = "summary";
                    root.controller.summarizeMemo(root.currentMemo.id);
                }
            }
        }
        CachyMenuSeparator {}
        CachyMenuItem {
            text: qsTr("Duplicar grabación")
            iconName: "copy"
            enabled: root.hasSelection && !root.isRecording
            onTriggered: if (root.controller && root.hasSelection) root.controller.duplicateMemo(root.currentMemo.id)
        }
        CachyMenuItem {
            text: qsTr("Exportar copia WAV…")
            iconName: "download"
            enabled: root.hasSelection && !root.isRecording
            onTriggered: {
                if (root.hasSelection) {
                    root.pendingExportMemoId = root.currentMemo.id;
                    exportWavDialog.open();
                }
            }
        }
        CachyMenuSeparator {}
        CachyMenuItem {
            text: root.isTrashMemo ? qsTr("Eliminar definitivamente") : qsTr("Mover a Eliminadas")
            iconName: "trash"
            destructive: true
            enabled: root.hasSelection && !root.isRecording
            onTriggered: if (root.controller && root.hasSelection) root.controller.moveToTrash(root.currentMemo.id)
        }
    }

    // =========================================================================
    // 5. GOLDEN GATE FLOATING POPOVERS (Playback Settings & Share)
    // =========================================================================
    CachyPopover {
        id: playbackPopover
        objectName: "playbackPopover"
        preferredEdge: Qt.BottomEdge
        preferredWidth: 316

        onOpened: {
            if (root.controller) {
                skipSilenceCheck.checked = root.controller.skipSilence;
                enhanceRecCheck.checked = root.controller.enhanceRecording;
                autoTranscribeCheck.checked = root.controller.autoTranscribeOnStop;
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Velocidad de reproducción")
                font.pixelSize: 13
                font.weight: Font.Bold
                color: CachyPalette.text
                elide: Text.ElideRight
            }

            Text {
                text: (root.controller ? root.controller.playbackSpeed : 1.0).toFixed(2).replace(/\.00$/, "") + "×"
                font.pixelSize: 12
                font.weight: Font.Bold
                color: CachyPalette.accentText
            }
        }

        CachySlider {
            Layout.fillWidth: true
            from: 0.5
            to: 2.0
            stepSize: 0.25
            value: root.controller ? root.controller.playbackSpeed : 1.0
            onMoved: if (root.controller) root.controller.playbackSpeed = value
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: [0.5, 1.0, 1.5, 2.0]

                CachyButton {
                    required property real modelData
                    Layout.fillWidth: true
                    leftPadding: 8
                    rightPadding: 8
                    text: modelData + "×"
                    primary: root.controller && Math.abs(root.controller.playbackSpeed - modelData) < 0.05
                    pillStyle: true
                    onClicked: if (root.controller) root.controller.playbackSpeed = modelData
                }
            }
        }

        CachyFormCard {
            Layout.fillWidth: true
            Layout.minimumWidth: 0

            CachyFormCheckDelegate {
                id: skipSilenceCheck
                label: qsTr("Omitir silencios")
                description: qsTr("Salta pausas largas durante la reproducción")
                checked: root.controller ? root.controller.skipSilence : false
                onToggled: checked => {
                    if (root.controller) root.controller.skipSilence = checked;
                }
            }
            CachyFormDelegateSeparator {}
            CachyFormCheckDelegate {
                id: enhanceRecCheck
                label: qsTr("Mejorar grabación")
                description: qsTr("Reduce ruido ambiente y realza la voz")
                checked: root.controller ? root.controller.enhanceRecording : false
                onToggled: checked => {
                    if (root.controller) root.controller.enhanceRecording = checked;
                }
            }
            CachyFormDelegateSeparator {}
            CachyFormCheckDelegate {
                id: autoTranscribeCheck
                label: qsTr("Transcribir al finalizar")
                description: qsTr("Ejecuta Whisper.cpp tras detener la grabación")
                checked: root.controller ? root.controller.autoTranscribeOnStop : true
                onToggled: checked => {
                    if (root.controller) root.controller.autoTranscribeOnStop = checked;
                }
            }
        }
    }

    CachyPopover {
        id: sharePopover
        objectName: "sharePopover"
        preferredEdge: Qt.BottomEdge
        preferredWidth: 304

        CachyCard {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            contentItem: RowLayout {
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 8
                    color: "#FF3B30"

                    CachyIcon {
                        anchors.centerIn: parent
                        name: "waveform"
                        size: 16
                        color: "#FFFFFF"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 1

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: root.hasSelection ? (root.currentMemo.title || qsTr("Nota de voz")) : qsTr("Nota de voz")
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: CachyPalette.text
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: root.controller && root.hasSelection
                            ? root.controller.formatDuration(root.currentMemo.durationSec || 0)
                            : "00:00"
                        font.pixelSize: 11
                        color: CachyPalette.textSecondary
                        elide: Text.ElideRight
                    }
                }
            }
        }

        CachyFormCard {
            Layout.fillWidth: true
            Layout.minimumWidth: 0

            CachyFormButtonDelegate {
                text: qsTr("Exportar a Kity Knowledge RAG")
                icon.name: "sparkles"
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.exportToKityKnowledge(root.currentMemo.id);
                    }
                    sharePopover.close();
                }
            }
            CachyFormDelegateSeparator {}
            CachyFormButtonDelegate {
                text: qsTr("Copiar transcripción")
                icon.name: "copy"
                enabled: Boolean(root.hasSelection && root.currentMemo.transcript && root.currentMemo.transcript.length > 0)
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.copyText(root.currentMemo.transcript || "");
                    }
                    sharePopover.close();
                }
            }
            CachyFormDelegateSeparator {}
            CachyFormButtonDelegate {
                text: qsTr("Copiar resumen IA")
                icon.name: "copy"
                enabled: Boolean(root.hasSelection && root.currentMemo.summary && root.currentMemo.summary.length > 0)
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        root.controller.copyText(root.currentMemo.summary || "");
                    }
                    sharePopover.close();
                }
            }
            CachyFormDelegateSeparator {}
            CachyFormButtonDelegate {
                text: qsTr("Exportar copia WAV…")
                icon.name: "download"
                onClicked: {
                    sharePopover.close();
                    if (root.hasSelection) {
                        root.pendingExportMemoId = root.currentMemo.id;
                        exportWavDialog.open();
                    }
                }
            }
        }
    }
}
