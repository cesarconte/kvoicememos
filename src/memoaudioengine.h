/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QVariantList>

class MemoAudioEngine : public QObject
{
    Q_OBJECT

public:
    explicit MemoAudioEngine(QObject *parent = nullptr);
    ~MemoAudioEngine() override;

    bool isRecording() const;
    int recordingSeconds() const;
    int recordingElapsedMs() const;
    double currentLevel() const;
    QVariantList liveWaveform() const;

    QString playingMemoId() const;
    double playbackPositionSec() const;
    double playbackDurationSec() const;
    bool playbackPaused() const;
    double playbackSpeed() const;
    bool skipSilence() const;
    bool enhanceRecording() const;

    void startRecording(const QString &storageDir, const QString &customTitle = QString());
    void stopRecording();

    void playMemo(const QString &id, const QString &filePath, const QString &title, double startSec = -1.0);
    void togglePlayPause(const QString &id, const QString &filePath, const QString &title);
    void stopPlayback();
    void seekPlayback(double seconds, const QString &id, const QString &filePath, const QString &title);
    void skipPlayback(double deltaSeconds, const QString &id, const QString &filePath, const QString &title);

    void setPlaybackSpeed(double speed);
    void setSkipSilence(bool enabled);
    void setEnhanceRecording(bool enabled);

    static int computeWavDurationSeconds(const QString &wavPath);
    static QByteArray createSyntheticWav(int durationSeconds, int sampleRate = 16000);
    static QVariantList extractWaveformPeaks(const QString &wavPath, int barCount = 140);
    static bool trimWavFile(const QString &srcPath, const QString &destPath, double startSec, double endSec);

Q_SIGNALS:
    void recordingChanged(bool recording);
    void recordingSecondsChanged(int seconds);
    void recordingElapsedMsChanged(int elapsedMs);
    void currentLevelChanged(double level);
    void liveWaveformChanged();
    void playingMemoIdChanged(const QString &id);
    void playbackPositionSecChanged(double pos);
    void playbackDurationSecChanged(double dur);
    void playbackPausedChanged(bool paused);
    void playbackSpeedChanged(double speed);
    void skipSilenceChanged(bool skip);
    void enhanceRecordingChanged(bool enhance);
    void recordingSaved(const QString &id, const QString &title, const QString &filePath, int durationSec);
    void statusRequested(const QString &message);

private Q_SLOTS:
    void updateRecordingMetrics();
    void updatePlaybackMetrics();

private:
    void startPlaybackProcess(const QString &id, const QString &filePath, const QString &title, double startSec);
    void killPlaybackProcess();

    bool m_recording = false;
    int m_recordingSeconds = 0;
    int m_recordingElapsedMs = 0;
    double m_currentLevel = 0.0;
    QVariantList m_liveWaveform;

    QString m_activeRecordingId;
    QString m_activeRecordingTitle;
    QString m_activeRecordingPath;

    QString m_playingMemoId;
    QString m_playingFilePath;
    QString m_playingTitle;
    double m_playbackPositionSec = 0.0;
    double m_playbackDurationSec = 0.0;
    bool m_playbackPaused = false;
    double m_playbackSpeed = 1.0;
    bool m_skipSilence = false;
    bool m_enhanceRecording = false;

    QProcess *m_recordProcess = nullptr;
    QProcess *m_playProcess = nullptr;
    QTimer *m_recordTimer = nullptr;
    QTimer *m_playTimer = nullptr;
};
