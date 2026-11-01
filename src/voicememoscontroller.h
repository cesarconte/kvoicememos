/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class VoiceMemosController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList memos READ memos NOTIFY memosChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(int recordingSeconds READ recordingSeconds NOTIFY recordingSecondsChanged)
    Q_PROPERTY(double currentLevel READ currentLevel NOTIFY currentLevelChanged)
    Q_PROPERTY(QString playingMemoId READ playingMemoId NOTIFY playingMemoIdChanged)
    Q_PROPERTY(QString selectedMemoId READ selectedMemoId NOTIFY selectedMemoIdChanged)
    Q_PROPERTY(QVariantMap selectedMemo READ selectedMemo NOTIFY selectedMemoChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit VoiceMemosController(const QString &storageDir = QString(),
                                  const QString &knowledgeDir = QString(),
                                  QObject *parent = nullptr);
    ~VoiceMemosController() override;

    QVariantList memos() const;
    bool recording() const;
    int recordingSeconds() const;
    double currentLevel() const;
    QString playingMemoId() const;
    QString selectedMemoId() const;
    QVariantMap selectedMemo() const;
    QString statusMessage() const;

    QString storageDir() const;
    void setStorageDir(const QString &dir);

    QString knowledgeDir() const;
    void setKnowledgeDir(const QString &dir);

    void setAutoTranscribeOnStop(bool enabled);
    bool autoTranscribeOnStop() const;

    static QString cleanWhisperOutput(const QString &raw);
    static int computeWavDurationSeconds(const QString &wavPath);
    static QByteArray createSyntheticWav(int durationSeconds, int sampleRate = 16000);
    static QString slugifyTitle(const QString &title);

    Q_INVOKABLE void startRecording(const QString &customTitle = QString());
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void playMemo(const QString &id);
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void transcribeMemo(const QString &id);
    Q_INVOKABLE void summarizeMemo(const QString &id);
    Q_INVOKABLE void renameMemo(const QString &id, const QString &newTitle);
    Q_INVOKABLE void deleteMemo(const QString &id);
    Q_INVOKABLE bool exportToKiriKnowledge(const QString &id);
    Q_INVOKABLE void selectMemo(const QString &id);
    Q_INVOKABLE void filterMemos(const QString &searchQuery);
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE QString formatDuration(int seconds) const;

    // Helper for importing existing WAV files or unit testing
    Q_INVOKABLE QString addMemoFromWav(const QString &wavPath,
                                       const QString &title = QString(),
                                       const QString &transcript = QString(),
                                       const QString &summary = QString());

    bool loadMemos();
    bool saveMemos() const;

Q_SIGNALS:
    void memosChanged();
    void recordingChanged();
    void recordingSecondsChanged();
    void currentLevelChanged();
    void playingMemoIdChanged();
    void selectedMemoIdChanged();
    void selectedMemoChanged();
    void statusMessageChanged();

private:
    void updateRecordingMetrics();
    void setStatus(const QString &msg);
    int findMemoIndex(const QString &id) const;
    void updateMemoField(const QString &id, const QString &key, const QVariant &value, bool persist = true);
    QString resolveWhisperBinary() const;
    QString resolveWhisperModel() const;

    QString m_storageDir;
    QString m_knowledgeDir;
    QString m_searchQuery;
    QVariantList m_allMemos;

    bool m_recording = false;
    int m_recordingSeconds = 0;
    int m_recordingTickCount = 0;
    double m_currentLevel = 0.0;
    QString m_activeRecordingId;
    QString m_activeRecordingTitle;
    QString m_activeRecordingPath;
    bool m_autoTranscribeOnStop = true;

    QString m_playingMemoId;
    QString m_selectedMemoId;
    QString m_statusMessage;

    QProcess *m_recordProcess = nullptr;
    QProcess *m_playProcess = nullptr;
    QTimer *m_recordTimer = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
};
