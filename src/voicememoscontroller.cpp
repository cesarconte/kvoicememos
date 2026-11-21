/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "voicememoscontroller.h"
#include "memoaudioengine.h"
#include "memostore.h"
#include "memotranscriptionservice.h"

#include <algorithm>

#include <QClipboard>
#include <QGuiApplication>

VoiceMemosController::VoiceMemosController(const QString &storageDir,
                                           const QString &knowledgeDir,
                                           QObject *parent)
    : QObject(parent)
    , m_audioEngine(new MemoAudioEngine(this))
    , m_transcriptionService(new MemoTranscriptionService(this))
    , m_memoStore(new MemoStore(storageDir, knowledgeDir, this))
{
    // Audio engine signals
    connect(m_audioEngine, &MemoAudioEngine::recordingChanged, this, &VoiceMemosController::recordingChanged);
    connect(m_audioEngine, &MemoAudioEngine::recordingSecondsChanged, this, &VoiceMemosController::recordingSecondsChanged);
    connect(m_audioEngine, &MemoAudioEngine::recordingElapsedMsChanged, this, &VoiceMemosController::recordingElapsedMsChanged);
    connect(m_audioEngine, &MemoAudioEngine::currentLevelChanged, this, &VoiceMemosController::currentLevelChanged);
    connect(m_audioEngine, &MemoAudioEngine::liveWaveformChanged, this, &VoiceMemosController::liveWaveformChanged);
    connect(m_audioEngine, &MemoAudioEngine::playingMemoIdChanged, this, &VoiceMemosController::playingMemoIdChanged);
    connect(m_audioEngine, &MemoAudioEngine::playbackPositionSecChanged, this, &VoiceMemosController::playbackPositionSecChanged);
    connect(m_audioEngine, &MemoAudioEngine::playbackDurationSecChanged, this, &VoiceMemosController::playbackDurationSecChanged);
    connect(m_audioEngine, &MemoAudioEngine::playbackPausedChanged, this, &VoiceMemosController::playbackPausedChanged);
    connect(m_audioEngine, &MemoAudioEngine::playbackSpeedChanged, this, &VoiceMemosController::playbackSpeedChanged);
    connect(m_audioEngine, &MemoAudioEngine::skipSilenceChanged, this, &VoiceMemosController::skipSilenceChanged);
    connect(m_audioEngine, &MemoAudioEngine::enhanceRecordingChanged, this, &VoiceMemosController::enhanceRecordingChanged);
    connect(m_audioEngine, &MemoAudioEngine::statusRequested, this, &VoiceMemosController::setStatus);

    connect(m_audioEngine, &MemoAudioEngine::recordingSaved, this, [this](const QString &id, const QString &title, const QString &filePath, int durationSec) {
        m_memoStore->addMemo(id, title, filePath, durationSec);
        if (m_autoTranscribeOnStop) {
            transcribeMemo(id);
        }
    });

    // Transcription service signals
    connect(m_transcriptionService, &MemoTranscriptionService::transcriptionStarted, this, [this](const QString &id) {
        m_memoStore->updateMemoField(id, QStringLiteral("transcribing"), true, false);
    });
    connect(m_transcriptionService, &MemoTranscriptionService::transcriptionFinished, this, [this](const QString &id, const QString &text, bool success) {
        m_memoStore->updateMemoField(id, QStringLiteral("transcribing"), false, false);
        if (success) {
            m_memoStore->updateMemoField(id, QStringLiteral("transcript"), text, true);
        }
    });
    connect(m_transcriptionService, &MemoTranscriptionService::summaryStarted, this, [this](const QString &id) {
        m_memoStore->updateMemoField(id, QStringLiteral("summarizing"), true, false);
    });
    connect(m_transcriptionService, &MemoTranscriptionService::summaryFinished, this, [this](const QString &id, const QString &summary, bool success) {
        m_memoStore->updateMemoField(id, QStringLiteral("summarizing"), false, false);
        if (success) {
            m_memoStore->updateMemoField(id, QStringLiteral("summary"), summary, true);
        }
    });
    connect(m_transcriptionService, &MemoTranscriptionService::statusRequested, this, &VoiceMemosController::setStatus);

    // Memo store signals
    connect(m_memoStore, &MemoStore::memosChanged, this, &VoiceMemosController::memosChanged);
    connect(m_memoStore, &MemoStore::foldersChanged, this, &VoiceMemosController::foldersChanged);
    connect(m_memoStore, &MemoStore::selectedFolderChanged, this, &VoiceMemosController::selectedFolderChanged);
    connect(m_memoStore, &MemoStore::sortByChanged, this, &VoiceMemosController::sortByChanged);
    connect(m_memoStore, &MemoStore::searchQueryChanged, this, &VoiceMemosController::searchQueryChanged);
    connect(m_memoStore, &MemoStore::selectedMemoIdChanged, this, &VoiceMemosController::selectedMemoIdChanged);
    connect(m_memoStore, &MemoStore::selectedMemoChanged, this, &VoiceMemosController::selectedMemoChanged);
    connect(m_memoStore, &MemoStore::statusRequested, this, &VoiceMemosController::setStatus);
    connect(m_memoStore, &MemoStore::memoDeleted, this, [this](const QString &id) {
        if (m_audioEngine->playingMemoId() == id) {
            m_audioEngine->stopPlayback();
        }
    });
}

VoiceMemosController::~VoiceMemosController()
{
}

QVariantList VoiceMemosController::memos() const
{
    return m_memoStore->memos();
}

QVariantList VoiceMemosController::folders() const
{
    return m_memoStore->folders();
}

QString VoiceMemosController::selectedFolder() const
{
    return m_memoStore->selectedFolder();
}

void VoiceMemosController::setSelectedFolder(const QString &folder)
{
    m_memoStore->setSelectedFolder(folder);
}

QString VoiceMemosController::currentFolderTitle() const
{
    return m_memoStore->currentFolderTitle();
}

QString VoiceMemosController::sortBy() const
{
    return m_memoStore->sortBy();
}

void VoiceMemosController::setSortBy(const QString &sortBy)
{
    m_memoStore->setSortBy(sortBy);
}

QString VoiceMemosController::searchQuery() const
{
    return m_memoStore->searchQuery();
}

int VoiceMemosController::allCount() const
{
    return m_memoStore->allCount();
}

int VoiceMemosController::favoritesCount() const
{
    return m_memoStore->favoritesCount();
}

int VoiceMemosController::transcribedCount() const
{
    return m_memoStore->transcribedCount();
}

int VoiceMemosController::trashCount() const
{
    return m_memoStore->trashCount();
}

bool VoiceMemosController::recording() const
{
    return m_audioEngine->isRecording();
}

int VoiceMemosController::recordingSeconds() const
{
    return m_audioEngine->recordingSeconds();
}

int VoiceMemosController::recordingElapsedMs() const
{
    return m_audioEngine->recordingElapsedMs();
}

double VoiceMemosController::currentLevel() const
{
    return m_audioEngine->currentLevel();
}

QVariantList VoiceMemosController::liveWaveform() const
{
    return m_audioEngine->liveWaveform();
}

QString VoiceMemosController::playingMemoId() const
{
    return m_audioEngine->playingMemoId();
}

double VoiceMemosController::playbackPositionSec() const
{
    return m_audioEngine->playbackPositionSec();
}

double VoiceMemosController::playbackDurationSec() const
{
    return m_audioEngine->playbackDurationSec();
}

bool VoiceMemosController::playbackPaused() const
{
    return m_audioEngine->playbackPaused();
}

double VoiceMemosController::playbackSpeed() const
{
    return m_audioEngine->playbackSpeed();
}

void VoiceMemosController::setPlaybackSpeed(double speed)
{
    m_audioEngine->setPlaybackSpeed(speed);
}

bool VoiceMemosController::skipSilence() const
{
    return m_audioEngine->skipSilence();
}

void VoiceMemosController::setSkipSilence(bool skip)
{
    m_audioEngine->setSkipSilence(skip);
}

bool VoiceMemosController::enhanceRecording() const
{
    return m_audioEngine->enhanceRecording();
}

void VoiceMemosController::setEnhanceRecording(bool enhance)
{
    m_audioEngine->setEnhanceRecording(enhance);
}

QString VoiceMemosController::selectedMemoId() const
{
    return m_memoStore->selectedMemoId();
}

QVariantMap VoiceMemosController::selectedMemo() const
{
    return m_memoStore->selectedMemo();
}

QString VoiceMemosController::statusMessage() const
{
    return m_statusMessage;
}

QString VoiceMemosController::storageDir() const
{
    return m_memoStore->storageDir();
}

void VoiceMemosController::setStorageDir(const QString &dir)
{
    m_memoStore->setStorageDir(dir);
}

QString VoiceMemosController::knowledgeDir() const
{
    return m_memoStore->knowledgeDir();
}

void VoiceMemosController::setKnowledgeDir(const QString &dir)
{
    m_memoStore->setKnowledgeDir(dir);
}

void VoiceMemosController::setAutoTranscribeOnStop(bool enabled)
{
    if (m_autoTranscribeOnStop != enabled) {
        m_autoTranscribeOnStop = enabled;
        Q_EMIT autoTranscribeOnStopChanged();
    }
}

bool VoiceMemosController::autoTranscribeOnStop() const
{
    return m_autoTranscribeOnStop;
}

QString VoiceMemosController::cleanWhisperOutput(const QString &raw)
{
    return MemoTranscriptionService::cleanWhisperOutput(raw);
}

int VoiceMemosController::computeWavDurationSeconds(const QString &wavPath)
{
    return MemoAudioEngine::computeWavDurationSeconds(wavPath);
}

QByteArray VoiceMemosController::createSyntheticWav(int durationSeconds, int sampleRate)
{
    return MemoAudioEngine::createSyntheticWav(durationSeconds, sampleRate);
}

QVariantList VoiceMemosController::extractWaveformPeaks(const QString &wavPath, int barCount)
{
    return MemoAudioEngine::extractWaveformPeaks(wavPath, barCount);
}

bool VoiceMemosController::trimWavFile(const QString &srcPath, const QString &destPath, double startSec, double endSec)
{
    return MemoAudioEngine::trimWavFile(srcPath, destPath, startSec, endSec);
}

QString VoiceMemosController::slugifyTitle(const QString &title)
{
    return MemoStore::slugifyTitle(title);
}

void VoiceMemosController::startRecording(const QString &customTitle)
{
    m_audioEngine->startRecording(m_memoStore->storageDir(), customTitle);
}

void VoiceMemosController::stopRecording()
{
    m_audioEngine->stopRecording();
}

void VoiceMemosController::playMemo(const QString &id)
{
    const int idx = m_memoStore->findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const QVariantMap memo = m_memoStore->selectedMemoId() == id
        ? m_memoStore->selectedMemo()
        : [&]() {
              QVariantMap m;
              for (const QVariant &v : m_memoStore->memos()) {
                  if (v.toMap().value(QStringLiteral("id")).toString() == id) {
                      return v.toMap();
                  }
              }
              return m;
          }();
    QString filePath = memo.value(QStringLiteral("filePath")).toString();
    QString title = memo.value(QStringLiteral("title")).toString();
    if (filePath.isEmpty()) {
        // Fallback when memo isn't in currently filtered view (e.g. unit tests)
        const QString prev = m_memoStore->selectedMemoId();
        m_memoStore->selectMemo(id);
        const QVariantMap sel = m_memoStore->selectedMemo();
        filePath = sel.value(QStringLiteral("filePath")).toString();
        title = sel.value(QStringLiteral("title")).toString();
        if (!prev.isEmpty() && prev != id) {
            m_memoStore->selectMemo(prev);
        }
    }
    m_audioEngine->playMemo(id, filePath, title);
}

void VoiceMemosController::togglePlayPause(const QString &id)
{
    const QString prev = m_memoStore->selectedMemoId();
    if (prev != id) {
        m_memoStore->selectMemo(id);
    }
    const QVariantMap memo = m_memoStore->selectedMemo();
    if (memo.isEmpty()) {
        return;
    }
    m_audioEngine->togglePlayPause(
        id,
        memo.value(QStringLiteral("filePath")).toString(),
        memo.value(QStringLiteral("title")).toString());
}

void VoiceMemosController::stopPlayback()
{
    m_audioEngine->stopPlayback();
}

void VoiceMemosController::seekPlayback(const QString &id, double seconds)
{
    const QString targetId = id.isEmpty() ? m_memoStore->selectedMemoId() : id;
    if (m_memoStore->selectedMemoId() != targetId) {
        m_memoStore->selectMemo(targetId);
    }
    const QVariantMap memo = m_memoStore->selectedMemo();
    if (memo.isEmpty()) {
        return;
    }
    m_audioEngine->seekPlayback(
        seconds,
        targetId,
        memo.value(QStringLiteral("filePath")).toString(),
        memo.value(QStringLiteral("title")).toString());
}

void VoiceMemosController::skipPlayback(const QString &id, double deltaSeconds)
{
    const QString targetId = id.isEmpty() ? m_memoStore->selectedMemoId() : id;
    if (m_memoStore->selectedMemoId() != targetId) {
        m_memoStore->selectMemo(targetId);
    }
    const QVariantMap memo = m_memoStore->selectedMemo();
    if (memo.isEmpty()) {
        return;
    }
    m_audioEngine->skipPlayback(
        deltaSeconds,
        targetId,
        memo.value(QStringLiteral("filePath")).toString(),
        memo.value(QStringLiteral("title")).toString());
}

void VoiceMemosController::transcribeMemo(const QString &id)
{
    const int idx = m_memoStore->findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    if (m_memoStore->selectedMemoId() != id) {
        m_memoStore->selectMemo(id);
    }
    const QVariantMap memo = m_memoStore->selectedMemo();
    const QString wavPath = memo.value(QStringLiteral("filePath")).toString();
    m_transcriptionService->transcribe(id, wavPath);
}

void VoiceMemosController::summarizeMemo(const QString &id)
{
    const int idx = m_memoStore->findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    if (m_memoStore->selectedMemoId() != id) {
        m_memoStore->selectMemo(id);
    }
    const QVariantMap memo = m_memoStore->selectedMemo();
    const QString transcript = memo.value(QStringLiteral("transcript")).toString();
    m_transcriptionService->summarize(id, transcript);
}

void VoiceMemosController::renameMemo(const QString &id, const QString &newTitle)
{
    m_memoStore->renameMemo(id, newTitle);
}

void VoiceMemosController::deleteMemo(const QString &id)
{
    m_memoStore->deleteMemo(id);
}

void VoiceMemosController::toggleFavorite(const QString &id)
{
    m_memoStore->toggleFavorite(id);
}

void VoiceMemosController::moveToTrash(const QString &id)
{
    m_memoStore->moveToTrash(id);
}

void VoiceMemosController::restoreMemo(const QString &id)
{
    m_memoStore->restoreMemo(id);
}

void VoiceMemosController::emptyTrash()
{
    m_memoStore->emptyTrash();
}

QString VoiceMemosController::duplicateMemo(const QString &id)
{
    return m_memoStore->duplicateMemo(id);
}

bool VoiceMemosController::trimMemo(const QString &id, double startSec, double endSec, bool saveAsNew)
{
    return m_memoStore->trimMemo(id, startSec, endSec, saveAsNew);
}

bool VoiceMemosController::createFolder(const QString &name)
{
    return m_memoStore->createFolder(name);
}

bool VoiceMemosController::renameFolder(const QString &oldName, const QString &newName)
{
    return m_memoStore->renameFolder(oldName, newName);
}

bool VoiceMemosController::deleteFolder(const QString &name)
{
    return m_memoStore->deleteFolder(name);
}

void VoiceMemosController::moveMemoToFolder(const QString &id, const QString &folder)
{
    m_memoStore->moveMemoToFolder(id, folder);
}

bool VoiceMemosController::exportToKityKnowledge(const QString &id)
{
    return m_memoStore->exportToKityKnowledge(id);
}

bool VoiceMemosController::exportAudioCopy(const QString &id, const QString &destUrl)
{
    return m_memoStore->exportAudioCopy(id, destUrl);
}

QString VoiceMemosController::importWavFile(const QString &fileUrl)
{
    return m_memoStore->importWavFile(fileUrl);
}

void VoiceMemosController::selectMemo(const QString &id)
{
    m_memoStore->selectMemo(id);
}

void VoiceMemosController::filterMemos(const QString &searchQuery)
{
    m_memoStore->filterMemos(searchQuery);
}

void VoiceMemosController::copyText(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    if (QClipboard *cb = QGuiApplication::clipboard()) {
        cb->setText(text);
        setStatus(QStringLiteral("Texto copiado al portapapeles."));
    }
}

QString VoiceMemosController::formatDuration(int seconds) const
{
    const int mins = std::max(0, seconds) / 60;
    const int secs = std::max(0, seconds) % 60;
    return QStringLiteral("%1:%2")
        .arg(mins, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'));
}

QString VoiceMemosController::formatElapsedPrecise(int elapsedMs) const
{
    const int clamped = std::max(0, elapsedMs);
    const int totalSecs = clamped / 1000;
    const int mins = totalSecs / 60;
    const int secs = totalSecs % 60;
    const int centis = (clamped % 1000) / 10;
    return QStringLiteral("%1:%2.%3")
        .arg(mins, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'))
        .arg(centis, 2, 10, QLatin1Char('0'));
}

QString VoiceMemosController::addMemoFromWav(const QString &wavPath,
                                             const QString &title,
                                             const QString &transcript,
                                             const QString &summary)
{
    return m_memoStore->addMemoFromWav(wavPath, title, transcript, summary);
}

bool VoiceMemosController::loadMemos()
{
    return m_memoStore->loadMemos();
}

bool VoiceMemosController::saveMemos() const
{
    return m_memoStore->saveMemos();
}

void VoiceMemosController::setStatus(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        Q_EMIT statusMessageChanged();
    }
}
