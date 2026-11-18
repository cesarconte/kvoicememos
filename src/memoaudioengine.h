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

class MemoAudioEngine : public QObject
{
    Q_OBJECT

public:
    explicit MemoAudioEngine(QObject *parent = nullptr);
    ~MemoAudioEngine() override;

    bool isRecording() const;
    int recordingSeconds() const;
    double currentLevel() const;
    QString playingMemoId() const;

    void startRecording(const QString &storageDir, const QString &customTitle = QString());
    void stopRecording();

    void playMemo(const QString &id, const QString &filePath, const QString &title);
    void stopPlayback();

    static int computeWavDurationSeconds(const QString &wavPath);
    static QByteArray createSyntheticWav(int durationSeconds, int sampleRate = 16000);

Q_SIGNALS:
    void recordingChanged(bool recording);
    void recordingSecondsChanged(int seconds);
    void currentLevelChanged(double level);
    void playingMemoIdChanged(const QString &id);
    void recordingSaved(const QString &id, const QString &title, const QString &filePath, int durationSec);
    void statusRequested(const QString &message);

private Q_SLOTS:
    void updateRecordingMetrics();

private:
    bool m_recording = false;
    int m_recordingSeconds = 0;
    int m_recordingTickCount = 0;
    double m_currentLevel = 0.0;

    QString m_activeRecordingId;
    QString m_activeRecordingTitle;
    QString m_activeRecordingPath;

    QString m_playingMemoId;

    QProcess *m_recordProcess = nullptr;
    QProcess *m_playProcess = nullptr;
    QTimer *m_recordTimer = nullptr;
};
