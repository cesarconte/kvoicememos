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
#include <QFileInfo>
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
    connect(m_audioEngine, &MemoAudioEngine::currentLevelChanged, this, &VoiceMemosController::currentLevelChanged);
    connect(m_audioEngine, &MemoAudioEngine::playingMemoIdChanged, this, &VoiceMemosController::playingMemoIdChanged);
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

bool VoiceMemosController::recording() const
{
    return m_audioEngine->isRecording();
}

int VoiceMemosController::recordingSeconds() const
{
    return m_audioEngine->recordingSeconds();
}

double VoiceMemosController::currentLevel() const
{
    return m_audioEngine->currentLevel();
}

QString VoiceMemosController::playingMemoId() const
{
    return m_audioEngine->playingMemoId();
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
    m_autoTranscribeOnStop = enabled;
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
    const QVariantMap memo = m_memoStore->memos().at(idx).toMap();
    const QString filePath = memo.value(QStringLiteral("filePath")).toString();
    const QString title = memo.value(QStringLiteral("title")).toString();
    m_audioEngine->playMemo(id, filePath, title);
}

void VoiceMemosController::stopPlayback()
{
    m_audioEngine->stopPlayback();
}

void VoiceMemosController::transcribeMemo(const QString &id)
{
    const int idx = m_memoStore->findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const QVariantMap memo = m_memoStore->memos().at(idx).toMap();
    const QString wavPath = memo.value(QStringLiteral("filePath")).toString();
    m_transcriptionService->transcribe(id, wavPath);
}

void VoiceMemosController::summarizeMemo(const QString &id)
{
    const int idx = m_memoStore->findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const QVariantMap memo = m_memoStore->memos().at(idx).toMap();
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

bool VoiceMemosController::exportToKityKnowledge(const QString &id)
{
    return m_memoStore->exportToKityKnowledge(id);
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
