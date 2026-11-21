/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import CachyOS.UI

ColumnLayout {
    id: root

    required property var controller
    property bool creatingFolder: false
    property string renamingFolder: ""
    property string contextTargetFolder: ""

    readonly property string activeFolder: controller ? controller.selectedFolder : "all"

    function startCreateFolder() {
        root.creatingFolder = !root.creatingFolder;
        if (root.creatingFolder) {
            newFolderField.text = "";
            newFolderField.forceActiveFocus();
        }
    }

    spacing: 4

    // Smart Folders (macOS 27 Golden Gate Voice Memos)
    CachySidebarItem {
        text: qsTr("Todas las grabaciones")
        iconName: "waveform"
        iconColor: "#0A84FF"
        circularIconBadge: true
        accentSelected: false
        badgeText: root.controller ? String(root.controller.allCount) : "0"
        selected: root.activeFolder === "all"
        onClicked: if (root.controller) root.controller.selectedFolder = "all"
    }

    CachySidebarItem {
        text: qsTr("Favoritos")
        iconName: "heart-filled"
        iconColor: "#FF3B30"
        circularIconBadge: true
        accentSelected: false
        badgeText: root.controller ? String(root.controller.favoritesCount) : "0"
        selected: root.activeFolder === "favorites"
        onClicked: if (root.controller) root.controller.selectedFolder = "favorites"
    }

    CachySidebarItem {
        text: qsTr("Con transcripción")
        iconName: "message"
        iconColor: "#AF52DE"
        circularIconBadge: true
        accentSelected: false
        badgeText: root.controller ? String(root.controller.transcribedCount) : "0"
        selected: root.activeFolder === "transcribed"
        onClicked: if (root.controller) root.controller.selectedFolder = "transcribed"
    }

    CachySidebarItem {
        visible: root.controller && root.controller.trashCount > 0
        text: qsTr("Eliminadas")
        iconName: "trash"
        iconColor: "#8E8E93"
        circularIconBadge: true
        accentSelected: false
        badgeText: root.controller ? String(root.controller.trashCount) : "0"
        selected: root.activeFolder === "trash"
        onClicked: if (root.controller) root.controller.selectedFolder = "trash"
    }

    // Section: Carpetas personalizadas
    Text {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.leftMargin: 10
        Layout.topMargin: 12
        Layout.bottomMargin: 2
        text: qsTr("Carpetas")
        font.pixelSize: 11
        font.weight: Font.DemiBold
        color: CachyPalette.textSecondary
        elide: Text.ElideRight
    }

    QQC2.ScrollView {
        id: folderScroll
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: availableWidth
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
        clip: true

        ColumnLayout {
            width: folderScroll.availableWidth
            spacing: 2

            Repeater {
                model: root.controller ? root.controller.folders : []

                Item {
                    id: customFolderRow
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: root.renamingFolder === modelData.name
                        ? renameField.implicitHeight + 4
                        : folderItem.implicitHeight

                    CachySidebarItem {
                        id: folderItem
                        anchors.fill: parent
                        visible: root.renamingFolder !== customFolderRow.modelData.name
                        text: customFolderRow.modelData.name
                        iconName: "folder"
                        iconColor: "#0A84FF"
                        circularIconBadge: true
                        accentSelected: false
                        badgeText: String(customFolderRow.modelData.count)
                        selected: root.activeFolder === customFolderRow.modelData.name
                        onClicked: if (root.controller) root.controller.selectedFolder = customFolderRow.modelData.name
                    }

                    MouseArea {
                        anchors.fill: parent
                        visible: root.renamingFolder !== customFolderRow.modelData.name
                        acceptedButtons: Qt.RightButton
                        onClicked: {
                            root.contextTargetFolder = customFolderRow.modelData.name;
                            folderContextMenu.popup();
                        }
                    }

                    CachyTextField {
                        id: renameField
                        anchors.fill: parent
                        anchors.topMargin: 2
                        anchors.bottomMargin: 2
                        visible: root.renamingFolder === customFolderRow.modelData.name
                        showClearButton: false
                        placeholderText: qsTr("Nombre de carpeta")
                        onVisibleChanged: {
                            if (visible) {
                                text = customFolderRow.modelData.name;
                                forceActiveFocus();
                                selectAll();
                            }
                        }
                        onAccepted: {
                            if (root.controller && text.trim().length > 0) {
                                root.controller.renameFolder(customFolderRow.modelData.name, text.trim());
                            }
                            root.renamingFolder = "";
                        }
                        Keys.onEscapePressed: root.renamingFolder = ""
                    }
                }
            }

            // Inline New Folder Creation Row (no modal dialog)
            CachyTextField {
                id: newFolderField
                Layout.fillWidth: true
                Layout.topMargin: 2
                visible: root.creatingFolder
                placeholderText: qsTr("Nueva carpeta…")
                showClearButton: false
                onAccepted: {
                    const cleaned = text.trim();
                    if (cleaned.length > 0 && root.controller) {
                        if (root.controller.createFolder(cleaned)) {
                            root.controller.selectedFolder = cleaned;
                        }
                    }
                    text = "";
                    root.creatingFolder = false;
                }
                Keys.onEscapePressed: {
                    text = "";
                    root.creatingFolder = false;
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: 10
                Layout.topMargin: 4
                visible: root.controller && root.controller.folders.length === 0 && !root.creatingFolder
                text: qsTr("Pulsa ＋ para crear una carpeta")
                font.pixelSize: 11
                color: CachyPalette.textMuted
                wrapMode: Text.WordWrap
            }
        }
    }

    CachyMenu {
        id: folderContextMenu

        CachyMenuItem {
            text: qsTr("Renombrar carpeta")
            iconName: "compose"
            onTriggered: root.renamingFolder = root.contextTargetFolder
        }
        CachyMenuItem {
            text: qsTr("Eliminar carpeta")
            iconName: "trash"
            destructive: true
            onTriggered: {
                if (root.controller && root.contextTargetFolder.length > 0) {
                    root.controller.deleteFolder(root.contextTargetFolder);
                }
            }
        }
    }
}
