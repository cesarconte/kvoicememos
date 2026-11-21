/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import CachyOS.UI

Item {
    id: root

    signal startRecordingRequested()
    signal importAudioRequested()

    Layout.fillWidth: true
    Layout.fillHeight: true

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(380, parent.width - 48)
        spacing: 12

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 68
            Layout.preferredHeight: 68
            radius: 34
            color: Qt.rgba(1.0, 0.23, 0.19, 0.14)
            border.width: 1
            border.color: Qt.rgba(1.0, 0.23, 0.19, 0.34)

            CachyIcon {
                anchors.centerIn: parent
                name: "waveform"
                size: 30
                color: "#FF3B30"
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: qsTr("Selecciona o graba una nota de voz")
            font.pixelSize: 17
            font.weight: Font.Bold
            color: CachyPalette.text
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: qsTr("Las grabaciones se capturan con PipeWire, se transcriben localmente con Whisper.cpp y pueden resumirse o enviarse a la base de conocimiento RAG de Kity.")
            font.pixelSize: 12
            color: CachyPalette.textSecondary
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 4
            spacing: 10

            CachyButton {
                text: qsTr("Grabar Nota de Voz")
                iconName: "microphone"
                destructive: true
                pillStyle: true
                onClicked: root.startRecordingRequested()
            }

            CachyButton {
                text: qsTr("Importar WAV…")
                iconName: "upload"
                pillStyle: true
                onClicked: root.importAudioRequested()
            }
        }
    }
}
