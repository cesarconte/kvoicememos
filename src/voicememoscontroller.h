/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class MemoAudioEngine;
class MemoTranscriptionService;
class MemoStore;

class VoiceMemosController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList memos READ memos NOTIFY memosChanged)
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)
    Q_PROPERTY(QString selectedFolder READ selectedFolder WRITE setSelectedFolder NOTIFY selectedFolderChanged)
    Q_PROPERTY(QString currentFolderTitle READ currentFolderTitle NOTIFY selectedFolderChanged)
    Q_PROPERTY(QString sortBy READ sortBy WRITE setSortBy NOTIFY sortByChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE filterMemos NOTIFY searchQueryChanged)

    Q_PROPERTY(int allCount READ allCount NOTIFY memosChanged)
    Q_PROPERTY(int favoritesCount READ favoritesCount NOTIFY memosChanged)
    Q_PROPERTY(int transcribedCount READ transcribedCount NOTIFY memosChanged)
    Q_PROPERTY(int trashCount READ trashCount NOTIFY memosChanged)

    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(int recordingSeconds READ recordingSeconds NOTIFY recordingSecondsChanged)
    Q_PROPERTY(int recordingElapsedMs READ recordingElapsedMs NOTIFY recordingElapsedMsChanged)
    Q_PROPERTY(double currentLevel READ currentLevel NOTIFY currentLevelChanged)
    Q_PROPERTY(QVariantList liveWaveform READ liveWaveform NOTIFY liveWaveformChanged)

    Q_PROPERTY(QString playingMemoId READ playingMemoId NOTIFY playingMemoIdChanged)
    Q_PROPERTY(double playbackPositionSec READ playbackPositionSec NOTIFY playbackPositionSecChanged)
    Q_PROPERTY(double playbackDurationSec READ playbackDurationSec NOTIFY playbackDurationSecChanged)
    Q_PROPERTY(bool playbackPaused READ playbackPaused NOTIFY playbackPausedChanged)
    Q_PROPERTY(double playbackSpeed READ playbackSpeed WRITE setPlaybackSpeed NOTIFY playbackSpeedChanged)
    Q_PROPERTY(bool skipSilence READ skipSilence WRITE setSkipSilence NOTIFY skipSilenceChanged)
    Q_PROPERTY(bool enhanceRecording READ enhanceRecording WRITE setEnhanceRecording NOTIFY enhanceRecordingChanged)
    Q_PROPERTY(bool autoTranscribeOnStop READ autoTranscribeOnStop WRITE setAutoTranscribeOnStop NOTIFY autoTranscribeOnStopChanged)

    Q_PROPERTY(QString selectedMemoId READ selectedMemoId NOTIFY selectedMemoIdChanged)
    Q_PROPERTY(QVariantMap selectedMemo READ selectedMemo NOTIFY selectedMemoChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit VoiceMemosController(const QString &storageDir = QString(),
                                  const QString &knowledgeDir = QString(),
                                  QObject *parent = nullptr);
    ~VoiceMemosController() override;

    QVariantList memos() const;
    QVariantList folders() const;
    QString selectedFolder() const;
    void setSelectedFolder(const QString &folder);
    QString currentFolderTitle() const;
    QString sortBy() const;
    void setSortBy(const QString &sortBy);
    QString searchQuery() const;

    int allCount() const;
    int favoritesCount() const;
    int transcribedCount() const;
    int trashCount() const;

    bool recording() const;
    int recordingSeconds() const;
    int recordingElapsedMs() const;
    double currentLevel() const;
    QVariantList liveWaveform() const;

    QString playingMemoId() const;
    double playbackPositionSec() const;
    double playbackDurationSec() const;
    bool playbackPaused() const;
    double playbackSpeed() const;
    void setPlaybackSpeed(double speed);
    bool skipSilence() const;
    void setSkipSilence(bool skip);
    bool enhanceRecording() const;
    void setEnhanceRecording(bool enhance);

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
    static QVariantList extractWaveformPeaks(const QString &wavPath, int barCount = 140);
    static bool trimWavFile(const QString &srcPath, const QString &destPath, double startSec, double endSec);
    static QString slugifyTitle(const QString &title);

    Q_INVOKABLE void startRecording(const QString &customTitle = QString());
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void playMemo(const QString &id);
    Q_INVOKABLE void togglePlayPause(const QString &id);
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void seekPlayback(const QString &id, double seconds);
    Q_INVOKABLE void skipPlayback(const QString &id, double deltaSeconds);

    Q_INVOKABLE void transcribeMemo(const QString &id);
    Q_INVOKABLE void summarizeMemo(const QString &id);
    Q_INVOKABLE void renameMemo(const QString &id, const QString &newTitle);
    Q_INVOKABLE void deleteMemo(const QString &id);
    Q_INVOKABLE void toggleFavorite(const QString &id);
    Q_INVOKABLE void moveToTrash(const QString &id);
    Q_INVOKABLE void restoreMemo(const QString &id);
    Q_INVOKABLE void emptyTrash();
    Q_INVOKABLE QString duplicateMemo(const QString &id);
    Q_INVOKABLE bool trimMemo(const QString &id, double startSec, double endSec, bool saveAsNew);

    Q_INVOKABLE bool createFolder(const QString &name);
    Q_INVOKABLE bool renameFolder(const QString &oldName, const QString &newName);
    Q_INVOKABLE bool deleteFolder(const QString &name);
    Q_INVOKABLE void moveMemoToFolder(const QString &id, const QString &folder);

    Q_INVOKABLE bool exportToKityKnowledge(const QString &id);
    Q_INVOKABLE bool exportToKiriKnowledge(const QString &id) { return exportToKityKnowledge(id); }
    Q_INVOKABLE bool exportAudioCopy(const QString &id, const QString &destUrl);
    Q_INVOKABLE QString importWavFile(const QString &fileUrl);
    Q_INVOKABLE void selectMemo(const QString &id);
    Q_INVOKABLE void filterMemos(const QString &searchQuery);
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE QString formatDuration(int seconds) const;
    Q_INVOKABLE QString formatElapsedPrecise(int elapsedMs) const;

    // Helper for importing existing WAV files or unit testing
    Q_INVOKABLE QString addMemoFromWav(const QString &wavPath,
                                       const QString &title = QString(),
                                       const QString &transcript = QString(),
                                       const QString &summary = QString());

    bool loadMemos();
    bool saveMemos() const;

Q_SIGNALS:
    void memosChanged();
    void foldersChanged();
    void selectedFolderChanged();
    void sortByChanged();
    void searchQueryChanged();
    void recordingChanged();
    void recordingSecondsChanged();
    void recordingElapsedMsChanged();
    void currentLevelChanged();
    void liveWaveformChanged();
    void playingMemoIdChanged();
    void playbackPositionSecChanged();
    void playbackDurationSecChanged();
    void playbackPausedChanged();
    void playbackSpeedChanged();
    void skipSilenceChanged();
    void enhanceRecordingChanged();
    void autoTranscribeOnStopChanged();
    void selectedMemoIdChanged();
    void selectedMemoChanged();
    void statusMessageChanged();

private:
    void setStatus(const QString &msg);

    MemoAudioEngine *m_audioEngine = nullptr;
    MemoTranscriptionService *m_transcriptionService = nullptr;
    MemoStore *m_memoStore = nullptr;

    bool m_autoTranscribeOnStop = true;
    QString m_statusMessage;
};
