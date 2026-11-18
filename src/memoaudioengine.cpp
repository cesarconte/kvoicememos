/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "memoaudioengine.h"

#include <cmath>
#include <csignal>
#include <cstring>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

MemoAudioEngine::MemoAudioEngine(QObject *parent)
    : QObject(parent)
    , m_recordTimer(new QTimer(this))
{
    m_recordTimer->setInterval(200);
    connect(m_recordTimer, &QTimer::timeout, this, &MemoAudioEngine::updateRecordingMetrics);
}

MemoAudioEngine::~MemoAudioEngine()
{
    if (m_recording) {
        stopRecording();
    }
    stopPlayback();
}

bool MemoAudioEngine::isRecording() const
{
    return m_recording;
}

int MemoAudioEngine::recordingSeconds() const
{
    return m_recordingSeconds;
}

double MemoAudioEngine::currentLevel() const
{
    return m_currentLevel;
}

QString MemoAudioEngine::playingMemoId() const
{
    return m_playingMemoId;
}

void MemoAudioEngine::startRecording(const QString &storageDir, const QString &customTitle)
{
    if (m_recording) {
        return;
    }

    stopPlayback();

    const QString pwRecord = QStandardPaths::findExecutable(QStringLiteral("pw-record"));
    if (pwRecord.isEmpty()) {
        Q_EMIT statusRequested(QStringLiteral("Error: no se encontró pw-record (PipeWire) en el sistema."));
        return;
    }

    QDir().mkpath(storageDir);
    const QDateTime now = QDateTime::currentDateTime();
    m_activeRecordingId = now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    m_activeRecordingTitle = customTitle.trimmed().isEmpty()
        ? QStringLiteral("Nota de voz %1").arg(now.toString(QStringLiteral("dd/MM HH:mm")))
        : customTitle.trimmed();
    m_activeRecordingPath = storageDir + QStringLiteral("/memo_%1.wav").arg(m_activeRecordingId);

    m_recordProcess = new QProcess(this);
    QStringList args;
    args << QStringLiteral("--format=s16")
         << QStringLiteral("--rate=16000")
         << QStringLiteral("--channels=1")
         << m_activeRecordingPath;

    m_recordProcess->start(pwRecord, args);
    if (!m_recordProcess->waitForStarted(2000)) {
        Q_EMIT statusRequested(QStringLiteral("No se pudo iniciar la grabación con PipeWire."));
        m_recordProcess->deleteLater();
        m_recordProcess = nullptr;
        return;
    }

    m_recording = true;
    m_recordingSeconds = 0;
    m_recordingTickCount = 0;
    m_currentLevel = 0.05;
    m_recordTimer->start();

    Q_EMIT recordingChanged(m_recording);
    Q_EMIT recordingSecondsChanged(m_recordingSeconds);
    Q_EMIT currentLevelChanged(m_currentLevel);
    Q_EMIT statusRequested(QStringLiteral("Grabando desde PipeWire (16 kHz mono)…"));
}

void MemoAudioEngine::stopRecording()
{
    if (!m_recording) {
        return;
    }

    m_recordTimer->stop();

    if (m_recordProcess) {
        if (m_recordProcess->state() != QProcess::NotRunning) {
            const qint64 pid = m_recordProcess->processId();
            if (pid > 0) {
                ::kill(static_cast<pid_t>(pid), SIGINT);
            } else {
                m_recordProcess->terminate();
            }
            if (!m_recordProcess->waitForFinished(1500)) {
                m_recordProcess->terminate();
                if (!m_recordProcess->waitForFinished(800)) {
                    m_recordProcess->kill();
                }
            }
        }
        m_recordProcess->deleteLater();
        m_recordProcess = nullptr;
    }

    m_recording = false;
    m_currentLevel = 0.0;
    Q_EMIT recordingChanged(m_recording);
    Q_EMIT currentLevelChanged(m_currentLevel);

    QFileInfo fi(m_activeRecordingPath);
    if (!fi.exists() || fi.size() <= 44) {
        QFile::remove(m_activeRecordingPath);
        Q_EMIT statusRequested(QStringLiteral("Grabación descartada (archivo de audio vacío)."));
        return;
    }

    const int dur = std::max(1, computeWavDurationSeconds(m_activeRecordingPath));
    Q_EMIT recordingSaved(m_activeRecordingId, m_activeRecordingTitle, m_activeRecordingPath, dur);
    Q_EMIT statusRequested(QStringLiteral("Grabación guardada (%1 s).").arg(dur));
}

void MemoAudioEngine::playMemo(const QString &id, const QString &filePath, const QString &title)
{
    if (m_playingMemoId == id && m_playProcess && m_playProcess->state() != QProcess::NotRunning) {
        stopPlayback();
        return;
    }

    stopPlayback();

    if (!QFileInfo::exists(filePath)) {
        Q_EMIT statusRequested(QStringLiteral("Archivo WAV no encontrado: %1").arg(filePath));
        return;
    }

    const QString pwPlay = QStandardPaths::findExecutable(QStringLiteral("pw-play"));
    if (pwPlay.isEmpty()) {
        Q_EMIT statusRequested(QStringLiteral("Error: no se encontró pw-play en el sistema."));
        return;
    }

    m_playProcess = new QProcess(this);
    m_playingMemoId = id;
    Q_EMIT playingMemoIdChanged(m_playingMemoId);

    connect(m_playProcess, &QProcess::finished, this, [this, id](int, QProcess::ExitStatus) {
        if (m_playingMemoId == id) {
            m_playingMemoId.clear();
            Q_EMIT playingMemoIdChanged(m_playingMemoId);
        }
        if (m_playProcess) {
            m_playProcess->deleteLater();
            m_playProcess = nullptr;
        }
    });

    m_playProcess->start(pwPlay, {filePath});
    Q_EMIT statusRequested(QStringLiteral("Reproduciendo: %1").arg(title));
}

void MemoAudioEngine::stopPlayback()
{
    if (m_playProcess) {
        disconnect(m_playProcess, nullptr, this, nullptr);
        if (m_playProcess->state() != QProcess::NotRunning) {
            m_playProcess->terminate();
            if (!m_playProcess->waitForFinished(500)) {
                m_playProcess->kill();
            }
        }
        m_playProcess->deleteLater();
        m_playProcess = nullptr;
    }
    if (!m_playingMemoId.isEmpty()) {
        m_playingMemoId.clear();
        Q_EMIT playingMemoIdChanged(m_playingMemoId);
    }
}

int MemoAudioEngine::computeWavDurationSeconds(const QString &wavPath)
{
    QFileInfo fi(wavPath);
    if (!fi.exists() || fi.size() <= 44) {
        return 0;
    }
    const qint64 pcmBytes = fi.size() - 44;
    // 16000 Hz * 1 channel * 2 bytes (s16le) = 32000 bytes/sec
    return static_cast<int>(pcmBytes / (16000 * 2));
}

QByteArray MemoAudioEngine::createSyntheticWav(int durationSeconds, int sampleRate)
{
    const int channels = 1;
    const int bitsPerSample = 16;
    const int dataSize = std::max(0, durationSeconds) * sampleRate * channels * (bitsPerSample / 8);

    QByteArray wav(44 + dataSize, 0);
    char *h = wav.data();
    std::memcpy(h, "RIFF", 4);
    const qint32 chunkSize = 36 + dataSize;
    std::memcpy(h + 4, &chunkSize, 4);
    std::memcpy(h + 8, "WAVE", 4);
    std::memcpy(h + 12, "fmt ", 4);
    const qint32 subchunk1Size = 16;
    std::memcpy(h + 16, &subchunk1Size, 4);
    const qint16 audioFormat = 1; // PCM
    std::memcpy(h + 20, &audioFormat, 2);
    const qint16 numChannels = static_cast<qint16>(channels);
    std::memcpy(h + 22, &numChannels, 2);
    const qint32 sRate = sampleRate;
    std::memcpy(h + 24, &sRate, 4);
    const qint32 byteRate = sampleRate * channels * (bitsPerSample / 8);
    std::memcpy(h + 28, &byteRate, 4);
    const qint16 blockAlign = static_cast<qint16>(channels * (bitsPerSample / 8));
    std::memcpy(h + 32, &blockAlign, 2);
    const qint16 bitsPerSamp = static_cast<qint16>(bitsPerSample);
    std::memcpy(h + 34, &bitsPerSamp, 2);
    std::memcpy(h + 36, "data", 4);
    const qint32 dSize = dataSize;
    std::memcpy(h + 40, &dSize, 4);

    return wav;
}

void MemoAudioEngine::updateRecordingMetrics()
{
    if (!m_recording) {
        return;
    }

    ++m_recordingTickCount;
    if (m_recordingTickCount % 5 == 0) {
        ++m_recordingSeconds;
        Q_EMIT recordingSecondsChanged(m_recordingSeconds);
    }

    QFile f(m_activeRecordingPath);
    if (f.open(QIODevice::ReadOnly)) {
        const qint64 sz = f.size();
        if (sz > 44) {
            const qint64 bytesToRead = std::min<qint64>(sz - 44, 6400);
            f.seek(sz - bytesToRead);
            const QByteArray chunk = f.read(bytesToRead);
            const int sampleCount = chunk.size() / static_cast<int>(sizeof(qint16));
            if (sampleCount > 0) {
                const qint16 *samples = reinterpret_cast<const qint16 *>(chunk.constData());
                double sumSq = 0.0;
                for (int i = 0; i < sampleCount; ++i) {
                    const double norm = samples[i] / 32768.0;
                    sumSq += norm * norm;
                }
                const double rms = std::sqrt(sumSq / sampleCount);
                m_currentLevel = qBound(0.04, rms * 6.0, 1.0);
                Q_EMIT currentLevelChanged(m_currentLevel);
            }
        }
        f.close();
    }
}
