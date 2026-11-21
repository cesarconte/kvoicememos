/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import CachyOS.UI

ColumnLayout {
    id: root

    required property var controller
    required property var targetWindow

    signal importAudioRequested()
    signal exportAudioRequested(string memoId)

    property var contextTargetMemo: null

    readonly property var memosList: controller ? controller.memos : []
    readonly property bool isTrashView: controller && controller.selectedFolder === "trash"

    spacing: 0

    // 44px Column 2 Header (Folder Title + Count + (...) Pill)
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: CachyTheme.titleBarHeight
        z: 5

        DragHandler {
            target: null
            cursorShape: Qt.ClosedHandCursor
            onActiveChanged: if (active && root.targetWindow) root.targetWindow.startSystemMove()
        }
        TapHandler {
            enabled: Boolean(root.targetWindow && root.targetWindow.maximizable)
            onDoubleTapped: {
                if (!root.targetWindow) return;
                if (root.targetWindow.visibility === Window.Maximized) {
                    root.targetWindow.showNormal();
                } else {
                    root.targetWindow.showMaximized();
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: (root.targetWindow && !root.targetWindow.sidebarVisible) ? 16 : 14
            anchors.rightMargin: 12
            spacing: 8

            CachyTrafficLights {
                visible: Boolean(root.targetWindow && !root.targetWindow.sidebarVisible)
                window: root.targetWindow
            }

            CachySidebarToggleButton {
                visible: Boolean(root.targetWindow && !root.targetWindow.sidebarVisible)
                targetWindow: root.targetWindow
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: root.controller ? root.controller.currentFolderTitle : qsTr("Todas las grabaciones")
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: CachyPalette.text
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: root.memosList.length === 1
                        ? qsTr("1 grabación")
                        : qsTr("%1 grabaciones").arg(root.memosList.length)
                    font.pixelSize: 11
                    color: CachyPalette.textSecondary
                    elide: Text.ElideRight
                }
            }

            CachyToolbarButton {
                id: listMenuBtn
                iconName: "more-horizontal"
                tooltipText: qsTr("Ordenación e importación")
                onClicked: listOptionsMenu.popup(listMenuBtn, 0, listMenuBtn.height + 6)

                CachyMenu {
                    id: listOptionsMenu

                    CachyMenuItem {
                        text: qsTr("Ordenar por fecha")
                        checkable: true
                        checked: root.controller && root.controller.sortBy === "date"
                        onTriggered: if (root.controller) root.controller.sortBy = "date"
                    }
                    CachyMenuItem {
                        text: qsTr("Ordenar por título")
                        checkable: true
                        checked: root.controller && root.controller.sortBy === "title"
                        onTriggered: if (root.controller) root.controller.sortBy = "title"
                    }
                    CachyMenuItem {
                        text: qsTr("Ordenar por duración")
                        checkable: true
                        checked: root.controller && root.controller.sortBy === "duration"
                        onTriggered: if (root.controller) root.controller.sortBy = "duration"
                    }
                    CachyMenuSeparator {}
                    CachyMenuItem {
                        text: qsTr("Importar archivo WAV…")
                        iconName: "upload"
                        onTriggered: root.importAudioRequested()
                    }
                    CachyMenuSeparator {
                        visible: root.isTrashView && root.memosList.length > 0
                    }
                    CachyMenuItem {
                        visible: root.isTrashView && root.memosList.length > 0
                        text: qsTr("Vaciar Eliminadas")
                        iconName: "trash"
                        destructive: true
                        onTriggered: if (root.controller) root.controller.emptyTrash()
                    }
                }
            }
        }
    }

    // Pill Search Field
    CachySearchField {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.topMargin: 2
        Layout.bottomMargin: 8
        placeholderText: qsTr("Buscar grabaciones")
        text: root.controller ? root.controller.searchQuery : ""
        onTextChanged: {
            if (root.controller && root.controller.searchQuery !== text) {
                root.controller.filterMemos(text);
            }
        }
    }

    // Trash Notice Strip
    Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.bottomMargin: 6
        Layout.preferredHeight: 34
        radius: 8
        visible: root.isTrashView && root.memosList.length > 0
        color: CachyPalette.cardBackground
        border.width: 1
        border.color: CachyPalette.specularBorder

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 6

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Grabaciones en papelera")
                font.pixelSize: 11
                color: CachyPalette.textSecondary
                elide: Text.ElideRight
            }

            Text {
                text: qsTr("Vaciar")
                font.pixelSize: 11
                font.weight: Font.DemiBold
                color: "#FF3B30"
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler {
                    onTapped: if (root.controller) root.controller.emptyTrash()
                }
            }
        }
    }

    // Recordings ListView
    QQC2.ScrollView {
        id: listScroll
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: availableWidth
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
        clip: true

        ListView {
            id: memoListView
            width: listScroll.availableWidth
            spacing: 4
            leftMargin: 10
            rightMargin: 10
            bottomMargin: 10
            model: root.memosList

            delegate: Rectangle {
                id: delegateRoot
                required property var modelData

                width: memoListView.width - 20
                height: 64
                radius: 10

                readonly property bool isSelected: Boolean(root.controller
                    && root.controller.selectedMemoId === delegateRoot.modelData.id)
                readonly property bool isPlaying: Boolean(root.controller
                    && root.controller.playingMemoId === delegateRoot.modelData.id
                    && !root.controller.playbackPaused)
                readonly property bool isFavorite: Boolean(delegateRoot.modelData.favorite)
                readonly property bool hasTranscript: Boolean(delegateRoot.modelData.transcript && delegateRoot.modelData.transcript.length > 0)

                color: isSelected
                    ? Qt.rgba(0.04, 0.52, 1.0, CachyPalette.isDark ? 0.24 : 0.14)
                    : (rowHover.hovered ? CachyPalette.hoverOverlay : "transparent")
                border.width: isSelected ? 1 : 0
                border.color: isSelected ? CachyTheme.accentColor : "transparent"

                Behavior on color { ColorAnimation { duration: 120 } }

                HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: mouse => {
                        if (root.controller) {
                            root.controller.selectMemo(delegateRoot.modelData.id);
                        }
                        if (mouse.button === Qt.RightButton) {
                            root.contextTargetMemo = delegateRoot.modelData;
                            memoRowMenu.popup();
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 9

                    // Play / Pause Circular Button
                    Rectangle {
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        radius: 14
                        color: delegateRoot.isPlaying ? "#FF3B30" : CachyPalette.surfacePill
                        border.width: 1
                        border.color: delegateRoot.isPlaying ? "#FF3B30" : CachyPalette.specularBorder

                        CachyIcon {
                            anchors.centerIn: parent
                            name: delegateRoot.isPlaying ? "pause" : "play"
                            size: 12
                            color: delegateRoot.isPlaying ? "#FFFFFF" : CachyTheme.accentColor
                        }

                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                        TapHandler {
                            onTapped: {
                                if (root.controller) {
                                    root.controller.selectMemo(delegateRoot.modelData.id);
                                    root.controller.togglePlayPause(delegateRoot.modelData.id);
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 3

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 5

                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: delegateRoot.modelData.title || qsTr("Nota de voz")
                                font.pixelSize: 13
                                font.weight: delegateRoot.isSelected ? Font.Bold : Font.DemiBold
                                elide: Text.ElideRight
                                color: CachyPalette.text
                            }

                            CachyIcon {
                                visible: delegateRoot.isFavorite
                                name: "heart-filled"
                                size: 11
                                color: "#FF3B30"
                            }

                            Rectangle {
                                visible: delegateRoot.hasTranscript
                                Layout.preferredWidth: 6
                                Layout.preferredHeight: 6
                                radius: 3
                                color: "#AF52DE"
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: delegateRoot.modelData.createdAt || ""
                                font.pixelSize: 11
                                color: CachyPalette.textSecondary
                                elide: Text.ElideRight
                            }

                            Text {
                                text: root.controller
                                    ? root.controller.formatDuration(delegateRoot.modelData.durationSec || 0)
                                    : "00:00"
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                color: CachyPalette.textMuted
                            }
                        }
                    }
                }

                Rectangle {
                    visible: !delegateRoot.isSelected && !rowHover.hovered
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    height: 1
                    color: CachyPalette.dividerColor
                }
            }

            Text {
                anchors.centerIn: parent
                visible: memoListView.count === 0
                width: parent.width - 28
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: root.isTrashView
                    ? qsTr("La papelera está vacía.")
                    : qsTr("No hay grabaciones.\nPulsa el botón rojo inferior o el dock para grabar.")
                font.pixelSize: 12
                color: CachyPalette.textMuted
            }
        }
    }

    // Bottom Quick Record Button Strip (macOS Voice Memos Sidebar Footer)
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: CachyPalette.dividerColor
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 54

        Rectangle {
            id: sidebarRecBtn
            anchors.centerIn: parent
            width: 38
            height: 38
            radius: 19
            color: CachyPalette.surfacePill
            border.width: 2
            border.color: (root.controller && root.controller.recording)
                ? "#FF3B30"
                : (CachyPalette.isDark ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0, 0, 0, 0.16))

            scale: recBtnTap.pressed ? 0.92 : (recBtnHover.hovered ? 1.05 : 1.0)
            Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutBack } }

            Rectangle {
                anchors.centerIn: parent
                width: (root.controller && root.controller.recording) ? 14 : 26
                height: width
                radius: (root.controller && root.controller.recording) ? 4 : 13
                color: "#FF3B30"

                Behavior on width { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                Behavior on radius { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
            }

            HoverHandler {
                id: recBtnHover
                cursorShape: Qt.PointingHandCursor
            }
            TapHandler {
                id: recBtnTap
                onTapped: {
                    if (!root.controller) return;
                    if (root.controller.recording) {
                        root.controller.stopRecording();
                    } else {
                        root.controller.startRecording();
                    }
                }
            }
            QQC2.ToolTip.visible: recBtnHover.hovered
            QQC2.ToolTip.text: (root.controller && root.controller.recording)
                ? qsTr("Detener grabación")
                : qsTr("Grabar nueva nota de voz (Ctrl+N)")
            QQC2.ToolTip.delay: 450
        }
    }

    CachyMenu {
        id: memoRowMenu

        CachyMenuItem {
            text: (root.controller && root.contextTargetMemo && root.controller.playingMemoId === root.contextTargetMemo.id && !root.controller.playbackPaused)
                ? qsTr("Pausar")
                : qsTr("Reproducir")
            iconName: (root.controller && root.contextTargetMemo && root.controller.playingMemoId === root.contextTargetMemo.id && !root.controller.playbackPaused)
                ? "pause"
                : "play"
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.togglePlayPause(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuItem {
            text: (root.contextTargetMemo && root.contextTargetMemo.favorite)
                ? qsTr("Quitar de Favoritos")
                : qsTr("Añadir a Favoritos")
            iconName: "heart-filled"
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.toggleFavorite(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuSeparator {}
        CachyMenuItem {
            text: qsTr("Transcribir con Whisper.cpp")
            iconName: "message"
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.transcribeMemo(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuItem {
            text: qsTr("Resumir con IA (Ollama)")
            iconName: "sparkles"
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.summarizeMemo(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuItem {
            text: qsTr("Duplicar grabación")
            iconName: "copy"
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.duplicateMemo(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuItem {
            text: qsTr("Exportar a Kity RAG")
            iconName: "share"
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.exportToKityKnowledge(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuItem {
            text: qsTr("Exportar copia WAV…")
            iconName: "download"
            onTriggered: {
                if (root.contextTargetMemo) {
                    root.exportAudioRequested(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuItem {
            visible: Boolean(root.contextTargetMemo && root.contextTargetMemo.deleted)
            text: qsTr("Restaurar grabación")
            iconName: "refresh"
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.restoreMemo(root.contextTargetMemo.id);
                }
            }
        }
        CachyMenuSeparator {}
        CachyMenuItem {
            text: (root.contextTargetMemo && root.contextTargetMemo.deleted)
                ? qsTr("Eliminar definitivamente")
                : qsTr("Mover a Eliminadas")
            iconName: "trash"
            destructive: true
            onTriggered: {
                if (root.controller && root.contextTargetMemo) {
                    root.controller.moveToTrash(root.contextTargetMemo.id);
                }
            }
        }
    }
}
