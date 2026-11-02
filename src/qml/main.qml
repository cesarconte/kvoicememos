/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: "Notas de Voz (KVoiceMemos)"
    width: 980
    height: 660
    minimumWidth: 760
    minimumHeight: 500
    visible: true

    readonly property var controller: typeof VoiceMemos !== "undefined" ? VoiceMemos : null
    readonly property var currentMemo: controller ? controller.selectedMemo : ({})
    readonly property bool hasSelection: currentMemo && currentMemo.id !== undefined && currentMemo.id !== ""

    Item {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            // ==================== PANEL IZQUIERDO: LISTA Y GRABADORA ====================
            LiquidGlassCard {
                Layout.preferredWidth: 340
                Layout.minimumWidth: 300
                Layout.maximumWidth: 380
                Layout.fillHeight: true
                glassStyle: "shell"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    // Cabecera
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: "audio-input-microphone"
                            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                            color: Kirigami.Theme.highlightColor
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            QQC2.Label {
                                text: "Notas de Voz"
                                font.pixelSize: 17
                                font.bold: true
                                color: Kirigami.Theme.textColor
                            }

                            QQC2.Label {
                                text: "PipeWire • Whisper • Ollama"
                                font.pixelSize: 11
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }

                    // Barra de búsqueda
                    LiquidGlassCard {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        glassStyle: "recessed"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Kirigami.Units.mediumSpacing
                            anchors.rightMargin: Kirigami.Units.mediumSpacing
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Icon {
                                source: "search"
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                color: Kirigami.Theme.disabledTextColor
                            }

                            QQC2.TextField {
                                id: searchField
                                Layout.fillWidth: true
                                placeholderText: "Buscar por título, transcripción o resumen…"
                                background: null
                                color: Kirigami.Theme.textColor
                                placeholderTextColor: Kirigami.Theme.disabledTextColor
                                onTextChanged: {
                                    if (root.controller) {
                                        root.controller.filterMemos(text)
                                    }
                                }
                            }
                        }
                    }

                    // Lista de grabaciones
                    ListView {
                        id: memoListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Kirigami.Units.smallSpacing
                        model: root.controller ? root.controller.memos : []

                        delegate: Item {
                            id: delegateRoot
                            required property var modelData
                            width: ListView.view ? ListView.view.width : 300
                            height: 76

                            readonly property bool isSelected: root.controller
                                && root.controller.selectedMemoId === delegateRoot.modelData.id
                            readonly property bool isPlaying: root.controller
                                && root.controller.playingMemoId === delegateRoot.modelData.id

                            LiquidGlassCard {
                                anchors.fill: parent
                                glassStyle: "card"
                                isHighlighted: delegateRoot.isSelected

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (root.controller) {
                                            root.controller.selectMemo(delegateRoot.modelData.id)
                                        }
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: Kirigami.Units.mediumSpacing
                                    spacing: Kirigami.Units.smallSpacing

                                    QQC2.ToolButton {
                                        icon.name: delegateRoot.isPlaying ? "media-playback-stop" : "media-playback-start"
                                        QQC2.ToolTip.visible: hovered
                                        QQC2.ToolTip.text: delegateRoot.isPlaying ? "Detener" : "Reproducir"
                                        onClicked: {
                                            if (root.controller) {
                                                root.controller.selectMemo(delegateRoot.modelData.id)
                                                root.controller.playMemo(delegateRoot.modelData.id)
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        QQC2.Label {
                                            Layout.fillWidth: true
                                            text: delegateRoot.modelData.title || "Nota de voz"
                                            font.pixelSize: 13
                                            font.bold: true
                                            elide: Text.ElideRight
                                            color: Kirigami.Theme.textColor
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: Kirigami.Units.smallSpacing

                                            QQC2.Label {
                                                text: delegateRoot.modelData.createdAt || ""
                                                font.pixelSize: 11
                                                color: Kirigami.Theme.disabledTextColor
                                            }

                                            Rectangle {
                                                radius: 6
                                                color: Kirigami.Theme.alternateBackgroundColor
                                                border.width: 1
                                                border.color: Kirigami.Theme.highlightColor
                                                implicitWidth: durLabel.implicitWidth + 10
                                                implicitHeight: durLabel.implicitHeight + 4

                                                QQC2.Label {
                                                    id: durLabel
                                                    anchors.centerIn: parent
                                                    text: root.controller
                                                        ? root.controller.formatDuration(delegateRoot.modelData.durationSec || 0)
                                                        : "00:00"
                                                    font.pixelSize: 10
                                                    font.bold: true
                                                    color: Kirigami.Theme.highlightColor
                                                }
                                            }
                                        }
                                    }

                                    QQC2.ToolButton {
                                        icon.name: "edit-delete"
                                        QQC2.ToolTip.visible: hovered
                                        QQC2.ToolTip.text: "Eliminar nota"
                                        onClicked: {
                                            if (root.controller) {
                                                root.controller.deleteMemo(delegateRoot.modelData.id)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        QQC2.Label {
                            anchors.centerIn: parent
                            visible: memoListView.count === 0
                            width: parent.width - 32
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            text: "No hay notas de voz.\nPulsa «Grabar» abajo para capturar con PipeWire."
                            font.pixelSize: 12
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }

                    // Dock inferior de grabación estilo macOS Voice Memos
                    LiquidGlassCard {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 108
                        glassStyle: "recessed"
                        isHighlighted: root.controller ? root.controller.recording : false

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.mediumSpacing
                            spacing: Kirigami.Units.smallSpacing

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.mediumSpacing

                                // Indicador de tiempo en vivo
                                QQC2.Label {
                                    text: root.controller
                                        ? root.controller.formatDuration(root.controller.recordingSeconds)
                                        : "00:00"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: (root.controller && root.controller.recording) ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
                                }

                                // Barra de nivel de audio en vivo
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 10
                                    radius: 5
                                    color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.12)

                                    Rectangle {
                                        width: parent.width * (root.controller ? Math.min(1.0, root.controller.currentLevel) : 0.0)
                                        height: parent.height
                                        radius: 5
                                        color: (root.controller && root.controller.recording) ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.highlightColor
                                        Behavior on width {
                                            NumberAnimation { duration: 120 }
                                        }
                                    }
                                }
                            }

                            // Botón píldora rojo Grabar / Detener
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 42
                                radius: 21
                                color: (root.controller && root.controller.recording) ? "#d50032" : "#e53935"
                                border.width: 1
                                border.color: Qt.rgba(1, 1, 1, 0.35)

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: Kirigami.Units.smallSpacing

                                    Kirigami.Icon {
                                        source: (root.controller && root.controller.recording)
                                            ? "media-playback-stop"
                                            : "media-record"
                                        Layout.preferredWidth: 18
                                        Layout.preferredHeight: 18
                                        color: "#ffffff"
                                    }

                                    QQC2.Label {
                                        text: (root.controller && root.controller.recording)
                                            ? "Detener y transcribir"
                                            : "Nueva grabación"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#ffffff"
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (!root.controller) {
                                            return
                                        }
                                        if (root.controller.recording) {
                                            root.controller.stopRecording()
                                        } else {
                                            root.controller.startRecording()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ==================== PANEL DERECHO: DETALLES, WHISPER Y OLLAMA ====================
            LiquidGlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                glassStyle: "shell"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    // Cabecera de nota seleccionada + controles de título y reproducción
                    LiquidGlassCard {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 92
                        glassStyle: "card"
                        visible: root.hasSelection

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.mediumSpacing
                            spacing: Kirigami.Units.smallSpacing

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing

                                QQC2.TextField {
                                    id: titleEditor
                                    Layout.fillWidth: true
                                    text: root.currentMemo.title || ""
                                    placeholderText: "Título de la nota de voz…"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: Kirigami.Theme.textColor
                                    onAccepted: {
                                        if (root.controller && root.hasSelection) {
                                            root.controller.renameMemo(root.currentMemo.id, text)
                                        }
                                    }
                                }

                                QQC2.Button {
                                    icon.name: "edit-rename"
                                    text: "Guardar título"
                                    onClicked: {
                                        if (root.controller && root.hasSelection) {
                                            root.controller.renameMemo(root.currentMemo.id, titleEditor.text)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.mediumSpacing

                                QQC2.Button {
                                    icon.name: (root.controller && root.controller.playingMemoId === root.currentMemo.id)
                                        ? "media-playback-stop"
                                        : "media-playback-start"
                                    text: (root.controller && root.controller.playingMemoId === root.currentMemo.id)
                                        ? "Detener audio"
                                        : "Reproducir WAV"
                                    onClicked: {
                                        if (root.controller && root.hasSelection) {
                                            root.controller.playMemo(root.currentMemo.id)
                                        }
                                    }
                                }

                                QQC2.Label {
                                    text: root.hasSelection
                                        ? ("Fecha: " + (root.currentMemo.createdAt || "")
                                           + "  •  Duración: "
                                           + (root.controller ? root.controller.formatDuration(root.currentMemo.durationSec || 0) : "00:00"))
                                        : ""
                                    font.pixelSize: 12
                                    color: Kirigami.Theme.disabledTextColor
                                }

                                Item { Layout.fillWidth: true }
                            }
                        }
                    }

                    // Sección 1: Transcripción (Whisper.cpp)
                    LiquidGlassCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        glassStyle: "card"
                        visible: root.hasSelection

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.mediumSpacing
                            spacing: Kirigami.Units.smallSpacing

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing

                                QQC2.Label {
                                    text: "Transcripción (Whisper.cpp)"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: Kirigami.Theme.highlightColor
                                }

                                QQC2.BusyIndicator {
                                    running: Boolean(root.currentMemo.transcribing)
                                    visible: running
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 20
                                }

                                Item { Layout.fillWidth: true }

                                QQC2.Button {
                                    icon.name: "view-refresh"
                                    text: "Transcribir"
                                    enabled: !root.currentMemo.transcribing
                                    onClicked: {
                                        if (root.controller && root.hasSelection) {
                                            root.controller.transcribeMemo(root.currentMemo.id)
                                        }
                                    }
                                }

                                QQC2.Button {
                                    icon.name: "edit-copy"
                                    text: "Copiar"
                                    enabled: Boolean(root.currentMemo.transcript)
                                    onClicked: {
                                        if (root.controller) {
                                            root.controller.copyText(root.currentMemo.transcript || "")
                                        }
                                    }
                                }
                            }

                            QQC2.ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                QQC2.TextArea {
                                    readOnly: true
                                    wrapMode: TextEdit.Wrap
                                    text: root.currentMemo.transcript
                                        ? root.currentMemo.transcript
                                        : "Pulsa «Transcribir» para convertir la grabación en texto mediante Whisper.cpp local."
                                    color: root.currentMemo.transcript ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                                    font.pixelSize: 13
                                    background: null
                                }
                            }
                        }
                    }

                    // Sección 2: Resumen IA (Apple Intelligence / Ollama)
                    LiquidGlassCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        glassStyle: "card"
                        visible: root.hasSelection

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.mediumSpacing
                            spacing: Kirigami.Units.smallSpacing

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing

                                QQC2.Label {
                                    text: "Resumen IA (Apple Intelligence / Ollama)"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: Kirigami.Theme.highlightColor
                                }

                                QQC2.BusyIndicator {
                                    running: Boolean(root.currentMemo.summarizing)
                                    visible: running
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 20
                                }

                                Item { Layout.fillWidth: true }

                                QQC2.Button {
                                    icon.name: "view-refresh"
                                    text: "Resumir"
                                    enabled: !root.currentMemo.summarizing && Boolean(root.currentMemo.transcript)
                                    onClicked: {
                                        if (root.controller && root.hasSelection) {
                                            root.controller.summarizeMemo(root.currentMemo.id)
                                        }
                                    }
                                }

                                QQC2.Button {
                                    icon.name: "document-export"
                                    text: "Exportar a Kiri RAG"
                                    onClicked: {
                                        if (root.controller && root.hasSelection) {
                                            root.controller.exportToKiriKnowledge(root.currentMemo.id)
                                        }
                                    }
                                }
                            }

                            QQC2.ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                QQC2.TextArea {
                                    readOnly: true
                                    wrapMode: TextEdit.Wrap
                                    text: root.currentMemo.summary
                                        ? root.currentMemo.summary
                                        : "El resumen ejecutivo con viñetas y sugerencia de título (qwen2.5:7b) aparecerá aquí."
                                    color: root.currentMemo.summary ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                                    font.pixelSize: 13
                                    background: null
                                }
                            }
                        }
                    }

                    // Estado vacío cuando no hay nota seleccionada
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !root.hasSelection

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Kirigami.Units.mediumSpacing

                            Kirigami.Icon {
                                Layout.alignment: Qt.AlignHCenter
                                source: "audio-input-microphone"
                                Layout.preferredWidth: 56
                                Layout.preferredHeight: 56
                                color: Kirigami.Theme.highlightColor
                            }

                            QQC2.Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: "Selecciona o graba una nota de voz"
                                font.pixelSize: 16
                                font.bold: true
                                color: Kirigami.Theme.textColor
                            }

                            QQC2.Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: "Las grabaciones se transcriben localmente con Whisper.cpp y se resumen con Ollama."
                                font.pixelSize: 12
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }

                    // Barra de estado inferior
                    LiquidGlassCard {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        glassStyle: "recessed"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Kirigami.Units.mediumSpacing
                            anchors.rightMargin: Kirigami.Units.mediumSpacing

                            QQC2.Label {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: (root.controller && root.controller.statusMessage)
                                    ? root.controller.statusMessage
                                    : "Listo • PipeWire + Whisper.cpp + Ollama (qwen2.5:7b)"
                                font.pixelSize: 11
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }
                }
            }
        }
    }
}
