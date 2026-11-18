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
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
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
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
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
}
