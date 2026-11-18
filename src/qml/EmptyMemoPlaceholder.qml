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

    Layout.fillWidth: true
    Layout.fillHeight: true
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
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
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
