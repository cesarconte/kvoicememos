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
    required property var currentMemo
    required property bool hasSelection
    required property bool isCurrentPlaying
    property bool compactMode: false
    property bool trimMode: false
    signal cancelTrimRequested()
    signal trimDone()

    property real trimStartRatio: 0.1
    property real trimEndRatio: 0.9

    readonly property bool isRecording: Boolean(controller && controller.recording)
    readonly property double totalSeconds: {
        if (isRecording) {
            return Math.max(10.0, (controller ? controller.recordingElapsedMs : 0) / 1000.0 + 3.0);
        }
        const dur = currentMemo && currentMemo.durationSec ? Number(currentMemo.durationSec) : 0;
        return Math.max(1.0, dur);
    }
    readonly property double playheadSeconds: {
        if (isRecording) {
            return (controller ? controller.recordingElapsedMs : 0) / 1000.0;
        }
        if (controller && currentMemo && controller.playingMemoId === currentMemo.id) {
            return controller.playbackPositionSec;
        }
        return 0.0;
    }
    readonly property real playheadRatio: Math.max(0.0, Math.min(1.0, playheadSeconds / Math.max(1.0, totalSeconds)))
    readonly property var activeWaveform: {
        if (isRecording && controller && controller.liveWaveform) {
            return controller.liveWaveform;
        }
        if (currentMemo && currentMemo.waveform) {
            return currentMemo.waveform;
        }
        return [];
    }

    onTrimModeChanged: {
        if (trimMode) {
            trimStartRatio = 0.08;
            trimEndRatio = 0.92;
        }
        detailCanvas.requestPaint();
        overviewCanvas.requestPaint();
    }

    onActiveWaveformChanged: {
        detailCanvas.requestPaint();
        overviewCanvas.requestPaint();
    }
    onPlayheadRatioChanged: {
        detailCanvas.requestPaint();
        overviewCanvas.requestPaint();
    }
    onTotalSecondsChanged: rulerCanvas.requestPaint()
    onTrimStartRatioChanged: {
        detailCanvas.requestPaint();
        overviewCanvas.requestPaint();
    }
    onTrimEndRatioChanged: {
        detailCanvas.requestPaint();
        overviewCanvas.requestPaint();
    }

    spacing: 0

    // Inline Title + Date + Favorite Header (when viewing saved memo)
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 24
        Layout.rightMargin: 24
        Layout.topMargin: 2
        Layout.bottomMargin: 10
        spacing: 10
        visible: !root.isRecording && root.hasSelection

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: 2

            TextInput {
                id: titleEditor
                Layout.fillWidth: true
                text: root.currentMemo.title || ""
                font.pixelSize: 20
                font.weight: Font.Bold
                color: CachyPalette.text
                selectionColor: CachyTheme.accentColor
                selectedTextColor: "#FFFFFF"
                clip: true

                function commitRename() {
                    if (root.controller && root.hasSelection) {
                        const trimmed = text.trim();
                        if (trimmed.length > 0 && trimmed !== root.currentMemo.title) {
                            root.controller.renameMemo(root.currentMemo.id, trimmed);
                        } else if (trimmed.length === 0) {
                            text = root.currentMemo.title || "";
                        }
                    }
                }

                onAccepted: {
                    commitRename();
                    focus = false;
                }
                onActiveFocusChanged: {
                    if (!activeFocus) {
                        commitRename();
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Título de la grabación…")
                    color: CachyPalette.textMuted
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    visible: titleEditor.text.length === 0 && !titleEditor.activeFocus
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: (root.currentMemo.createdAt || "")
                    + "   ·   "
                    + (root.controller ? root.controller.formatDuration(root.currentMemo.durationSec || 0) : "00:00")
                    + (root.currentMemo.folder ? ("   ·   " + root.currentMemo.folder) : "")
                font.pixelSize: 12
                color: CachyPalette.textSecondary
                elide: Text.ElideRight
            }
        }

        // Favorite Heart Toggle
        CachyToolbarButton {
            iconName: root.currentMemo.favorite ? "heart-filled" : "heart"
            checked: Boolean(root.currentMemo.favorite)
            accentColor: "#FF3B30"
            tooltipText: root.currentMemo.favorite
                ? qsTr("Quitar de Favoritos")
                : qsTr("Añadir a Favoritos")
            onClicked: {
                if (root.controller && root.hasSelection) {
                    root.controller.toggleFavorite(root.currentMemo.id);
                }
            }
        }
    }

    // 1. Upper Full-Width Detail Waveform Band (27-Golden-Gate-Voice-Memos.png)
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: !root.compactMode
        Layout.preferredHeight: root.compactMode ? 156 : 260
        Layout.minimumHeight: 132
        color: CachyPalette.isDark ? "#17181D" : "#F5F5F7"
        clip: true

        // Top & bottom subtle hairlines framing the detail waveform band
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: CachyPalette.dividerColor
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: CachyPalette.dividerColor
        }

        Canvas {
            id: detailCanvas
            anchors.fill: parent
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d");
                const w = width;
                const h = height;
                ctx.clearRect(0, 0, w, h);
                if (w <= 4 || h <= 4) return;

                const midY = h / 2.0;

                // Subtle centerline
                ctx.fillStyle = CachyPalette.isDark ? "rgba(255,255,255,0.08)" : "rgba(0,0,0,0.06)";
                ctx.fillRect(0, midY - 0.5, w, 1);

                // Trim selection highlight
                if (root.trimMode && !root.isRecording) {
                    const tx0 = root.trimStartRatio * w;
                    const tx1 = root.trimEndRatio * w;
                    ctx.fillStyle = "rgba(255, 204, 0, 0.14)";
                    ctx.fillRect(tx0, 0, Math.max(2, tx1 - tx0), h);
                }

                const samples = root.activeWaveform || [];
                const barStep = 4.0;
                const barWidth = 2.0;
                const maxAmp = (h * 0.44);

                if (root.isRecording) {
                    // During recording: live waveform grows toward playhead
                    const headX = Math.max(12, Math.min(w - 24, root.playheadRatio * w));
                    const maxBars = Math.floor(headX / barStep);
                    const totalSamples = samples.length;
                    for (let i = 0; i < maxBars; ++i) {
                        const sampleIdx = totalSamples > maxBars
                            ? (totalSamples - maxBars + i)
                            : Math.min(totalSamples - 1, Math.floor((i / Math.max(1, maxBars)) * totalSamples));
                        const amp = totalSamples > 0 ? Number(samples[Math.max(0, sampleIdx)]) : 0.08;
                        const bh = Math.max(3.0, amp * maxAmp * 2.0);
                        const x = i * barStep;
                        ctx.fillStyle = "rgba(255, 79, 68, 0.85)";
                        ctx.fillRect(x, midY - bh / 2.0, barWidth, bh);
                    }
                } else if (samples.length > 0) {
                    const barsCount = Math.max(16, Math.floor(w / barStep));
                    const playX = root.playheadRatio * w;
                    for (let i = 0; i < barsCount; ++i) {
                        const x = i * barStep;
                        const idx = Math.min(samples.length - 1, Math.floor((i / barsCount) * samples.length));
                        const amp = Number(samples[idx]);
                        const bh = Math.max(3.0, amp * maxAmp * 2.0);
                        const played = (x <= playX && root.playheadRatio > 0.001);
                        ctx.fillStyle = played ? "#FF3B30" : "rgba(255, 92, 82, 0.72)";
                        ctx.fillRect(x, midY - bh / 2.0, barWidth, bh);
                    }
                }

                // Vertical Playhead Line with top & bottom circular caps
                const phX = root.isRecording
                    ? Math.max(12, Math.min(w - 24, root.playheadRatio * w))
                    : Math.max(1, Math.min(w - 1, root.playheadRatio * w));
                const phColor = root.isRecording ? "#FF3B30" : "#0A84FF";
                ctx.strokeStyle = phColor;
                ctx.lineWidth = 1.6;
                ctx.beginPath();
                ctx.moveTo(phX, 4);
                ctx.lineTo(phX, h - 4);
                ctx.stroke();

                ctx.fillStyle = phColor;
                ctx.beginPath();
                ctx.arc(phX, 5, 3.2, 0, Math.PI * 2);
                ctx.fill();
                ctx.beginPath();
                ctx.arc(phX, h - 5, 3.2, 0, Math.PI * 2);
                ctx.fill();
            }
        }

        // Scrubbing & Wheel Support
        MouseArea {
            anchors.fill: parent
            enabled: !root.isRecording && root.hasSelection && !root.trimMode
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

            function scrubTo(mx) {
                const ratio = Math.max(0.0, Math.min(1.0, mx / Math.max(1, width)));
                if (root.controller && root.hasSelection) {
                    root.controller.seekPlayback(root.currentMemo.id, ratio * root.totalSeconds);
                }
            }
            onClicked: mouse => scrubTo(mouse.x)
            onPositionChanged: mouse => {
                if (pressed) scrubTo(mouse.x);
            }
        }

        WheelHandler {
            enabled: !root.isRecording && root.hasSelection
            onWheel: event => {
                if (root.controller && root.hasSelection) {
                    const delta = (event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x) > 0 ? -0.5 : 0.5;
                    root.controller.skipPlayback(root.currentMemo.id, delta);
                }
            }
        }

        // Trim Left/Right Draggable Handles
        Item {
            id: trimHandlesLayer
            anchors.fill: parent
            visible: root.trimMode && !root.isRecording && root.hasSelection

            // Left Handle
            Rectangle {
                id: leftTrimHandle
                x: Math.max(0, Math.min(trimHandlesLayer.width - 28, root.trimStartRatio * trimHandlesLayer.width - 5))
                y: 0
                width: 10
                height: trimHandlesLayer.height
                color: "transparent"

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 3
                    height: parent.height
                    color: "#F0B817"
                }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 28
                    radius: 5
                    color: "#F0B817"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeHorCursor
                    onPositionChanged: mouse => {
                        if (!pressed) return;
                        const gx = leftTrimHandle.x + mouse.x + 5;
                        const r = Math.max(0.0, Math.min(root.trimEndRatio - 0.05, gx / Math.max(1, trimHandlesLayer.width)));
                        root.trimStartRatio = r;
                    }
                }
            }

            // Right Handle
            Rectangle {
                id: rightTrimHandle
                x: Math.max(18, Math.min(trimHandlesLayer.width - 10, root.trimEndRatio * trimHandlesLayer.width - 5))
                y: 0
                width: 10
                height: trimHandlesLayer.height
                color: "transparent"

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 3
                    height: parent.height
                    color: "#F0B817"
                }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 28
                    radius: 5
                    color: "#F0B817"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeHorCursor
                    onPositionChanged: mouse => {
                        if (!pressed) return;
                        const gx = rightTrimHandle.x + mouse.x + 5;
                        const r = Math.max(root.trimStartRatio + 0.05, Math.min(1.0, gx / Math.max(1, trimHandlesLayer.width)));
                        root.trimEndRatio = r;
                    }
                }
            }
        }
    }

    // 2. Time Ruler Strip directly under detail waveform
    Canvas {
        id: rulerCanvas
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d");
            const w = width;
            const h = height;
            ctx.clearRect(0, 0, w, h);
            if (w <= 20) return;

            const dur = root.totalSeconds;
            const stepSec = dur <= 12 ? 1 : (dur <= 60 ? 5 : (dur <= 300 ? 30 : 60));
            const tickColor = CachyPalette.isDark ? "rgba(255,255,255,0.20)" : "rgba(0,0,0,0.16)";
            const textColor = CachyPalette.isDark ? "rgba(255,255,255,0.45)" : "rgba(0,0,0,0.42)";

            ctx.font = "10px sans-serif";
            ctx.textAlign = "center";

            for (let s = 0; s <= dur; s += stepSec) {
                const x = (s / Math.max(1.0, dur)) * w;
                ctx.strokeStyle = tickColor;
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, 6);
                ctx.stroke();

                if (x > 16 && x < w - 16) {
                    const mins = Math.floor(s / 60);
                    const secs = Math.floor(s % 60);
                    const label = mins + ":" + (secs < 10 ? "0" + secs : secs);
                    ctx.fillStyle = textColor;
                    ctx.fillText(label, x, 17);
                }
            }
        }
    }

    // 3. Compact Overview Waveform Scrubber Bar (27-Golden-Gate-Voice-Memos.png)
    Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 24
        Layout.rightMargin: 24
        Layout.topMargin: 8
        Layout.preferredHeight: 40
        radius: 8
        color: CachyPalette.isDark ? "#23242B" : "#ECECEF"
        border.width: 1
        border.color: CachyPalette.specularBorder
        clip: true

        Canvas {
            id: overviewCanvas
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d");
                const w = width;
                const h = height;
                ctx.clearRect(0, 0, w, h);
                if (w <= 4 || h <= 4) return;

                const midY = h / 2.0;
                const samples = root.activeWaveform || [];
                const barStep = 3.0;

                if (root.trimMode && !root.isRecording) {
                    ctx.fillStyle = "rgba(240, 184, 23, 0.24)";
                    ctx.fillRect(root.trimStartRatio * w, 0, (root.trimEndRatio - root.trimStartRatio) * w, h);
                }

                if (root.isRecording) {
                    const headX = Math.max(6, root.playheadRatio * w);
                    const maxBars = Math.floor(headX / barStep);
                    const totalSamples = samples.length;
                    for (let i = 0; i < maxBars; ++i) {
                        const sampleIdx = totalSamples > maxBars
                            ? (totalSamples - maxBars + i)
                            : Math.min(totalSamples - 1, Math.floor((i / Math.max(1, maxBars)) * totalSamples));
                        const amp = totalSamples > 0 ? Number(samples[Math.max(0, sampleIdx)]) : 0.1;
                        const bh = Math.max(2.0, amp * h * 0.72);
                        ctx.fillStyle = "rgba(255, 84, 72, 0.78)";
                        ctx.fillRect(i * barStep, midY - bh / 2.0, 1.8, bh);
                    }
                    ctx.fillStyle = "#FF3B30";
                    ctx.fillRect(headX, 3, 2.5, h - 6);
                } else if (samples.length > 0) {
                    const barsCount = Math.max(12, Math.floor(w / barStep));
                    const playX = root.playheadRatio * w;
                    for (let i = 0; i < barsCount; ++i) {
                        const x = i * barStep;
                        const idx = Math.min(samples.length - 1, Math.floor((i / barsCount) * samples.length));
                        const amp = Number(samples[idx]);
                        const bh = Math.max(2.0, amp * h * 0.72);
                        ctx.fillStyle = (x <= playX && root.playheadRatio > 0.001)
                            ? "#FF3B30"
                            : "rgba(255, 95, 86, 0.65)";
                        ctx.fillRect(x, midY - bh / 2.0, 1.8, bh);
                    }
                    ctx.fillStyle = "#0A84FF";
                    ctx.fillRect(Math.max(0, Math.min(w - 2.5, playX)), 3, 2.5, h - 6);
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: !root.isRecording && root.hasSelection
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

            function scrubOverview(mx) {
                const ratio = Math.max(0.0, Math.min(1.0, mx / Math.max(1, width)));
                if (root.controller && root.hasSelection) {
                    root.controller.seekPlayback(root.currentMemo.id, ratio * root.totalSeconds);
                }
            }
            onClicked: mouse => scrubOverview(mouse.x)
            onPositionChanged: mouse => {
                if (pressed) scrubOverview(mouse.x);
            }
        }
    }

    // 4. Start / End Duration Labels under Overview Scrubber
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 24
        Layout.rightMargin: 24
        Layout.topMargin: 4
        Layout.bottomMargin: 6

        Text {
            text: "0:00"
            font.pixelSize: 10
            color: CachyPalette.textMuted
        }

        Item { Layout.fillWidth: true }

        Text {
            text: root.controller ? root.controller.formatDuration(Math.round(root.totalSeconds)) : "0:00"
            font.pixelSize: 10
            color: CachyPalette.textMuted
        }
    }

    // 5. Trim Action Strip (shown when trimMode is active)
    Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 24
        Layout.rightMargin: 24
        Layout.bottomMargin: 8
        Layout.preferredHeight: 42
        radius: 10
        visible: root.trimMode && !root.isRecording && root.hasSelection
        color: CachyPalette.cardBackground
        border.width: 1
        border.color: "#F0B817"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 10
            spacing: 8

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: {
                    if (!root.controller) return "";
                    const s0 = Math.round(root.trimStartRatio * root.totalSeconds);
                    const s1 = Math.round(root.trimEndRatio * root.totalSeconds);
                    return qsTr("Selección: %1 – %2")
                        .arg(root.controller.formatDuration(s0))
                        .arg(root.controller.formatDuration(s1));
                }
                font.pixelSize: 12
                font.weight: Font.Medium
                color: CachyPalette.text
                elide: Text.ElideRight
            }

            CachyButton {
                text: qsTr("Recortar")
                primary: true
                pillStyle: true
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        const s0 = root.trimStartRatio * root.totalSeconds;
                        const s1 = root.trimEndRatio * root.totalSeconds;
                        if (root.controller.trimMemo(root.currentMemo.id, s0, s1, false)) {
                            root.trimDone();
                        }
                    }
                }
            }

            CachyButton {
                text: qsTr("Guardar como nueva")
                pillStyle: true
                onClicked: {
                    if (root.controller && root.hasSelection) {
                        const s0 = root.trimStartRatio * root.totalSeconds;
                        const s1 = root.trimEndRatio * root.totalSeconds;
                        if (root.controller.trimMemo(root.currentMemo.id, s0, s1, true)) {
                            root.trimDone();
                        }
                    }
                }
            }

            CachyButton {
                text: qsTr("Cancelar")
                flat: true
                pillStyle: true
                onClicked: root.cancelTrimRequested()
            }
        }
    }
}
