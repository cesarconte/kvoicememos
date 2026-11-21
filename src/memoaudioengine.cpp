/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "memoaudioengine.h"

#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstring>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

struct WavHeaderInfo {
    bool valid = false;
    quint16 channels = 1;
    quint32 sampleRate = 16000;
    quint16 bitsPerSample = 16;
    quint16 blockAlign = 2;
    quint32 byteRate = 32000;
    qint64 dataOffset = 44;
    qint64 dataSize = 0;
};

WavHeaderInfo parseWavHeader(QFile &f)
{
    WavHeaderInfo info;
    const qint64 fileSize = f.size();
    if (fileSize <= 44) {
        return info;
    }

    f.seek(0);
    char prefix[12] = {};
    if (f.read(prefix, 12) != 12
        || std::memcmp(prefix, "RIFF", 4) != 0
        || std::memcmp(prefix + 8, "WAVE", 4) != 0) {
        return info;
    }

    qint64 pos = 12;
    bool foundFmt = false;
    bool foundData = false;

    while (pos + 8 <= fileSize) {
        f.seek(pos);
        char chunkHdr[8] = {};
        if (f.read(chunkHdr, 8) != 8) {
            break;
        }
        quint32 chunkSize = 0;
        std::memcpy(&chunkSize, chunkHdr + 4, 4);
        const qint64 payloadPos = pos + 8;

        if (std::memcmp(chunkHdr, "fmt ", 4) == 0 && chunkSize >= 16) {
            char fmt[16] = {};
            if (f.read(fmt, 16) == 16) {
                std::memcpy(&info.channels, fmt + 2, 2);
                std::memcpy(&info.sampleRate, fmt + 4, 4);
                std::memcpy(&info.byteRate, fmt + 8, 4);
                std::memcpy(&info.blockAlign, fmt + 12, 2);
                std::memcpy(&info.bitsPerSample, fmt + 14, 2);
                foundFmt = true;
            }
        } else if (std::memcmp(chunkHdr, "data", 4) == 0) {
            info.dataOffset = payloadPos;
            info.dataSize = std::min<qint64>(chunkSize, std::max<qint64>(0, fileSize - payloadPos));
            foundData = true;
            break;
        }

        pos = payloadPos + static_cast<qint64>(chunkSize) + (chunkSize & 1);
    }

    if (!foundFmt) {
        info.channels = 1;
        info.sampleRate = 16000;
        info.bitsPerSample = 16;
        info.blockAlign = 2;
        info.byteRate = 32000;
    }
    if (!foundData) {
        info.dataOffset = 44;
        info.dataSize = std::max<qint64>(0, fileSize - 44);
    }
    if (info.byteRate == 0) {
        info.byteRate = std::max<quint32>(1, info.sampleRate * info.channels * (info.bitsPerSample / 8));
    }
    if (info.blockAlign == 0) {
        info.blockAlign = std::max<quint16>(1, info.channels * (info.bitsPerSample / 8));
    }
    info.valid = info.dataSize > 0;
    return info;
}

} // namespace

MemoAudioEngine::MemoAudioEngine(QObject *parent)
    : QObject(parent)
    , m_recordTimer(new QTimer(this))
    , m_playTimer(new QTimer(this))
{
    m_recordTimer->setInterval(100);
    connect(m_recordTimer, &QTimer::timeout, this, &MemoAudioEngine::updateRecordingMetrics);

    m_playTimer->setInterval(50);
    connect(m_playTimer, &QTimer::timeout, this, &MemoAudioEngine::updatePlaybackMetrics);
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

int MemoAudioEngine::recordingElapsedMs() const
{
    return m_recordingElapsedMs;
}

double MemoAudioEngine::currentLevel() const
{
    return m_currentLevel;
}

QVariantList MemoAudioEngine::liveWaveform() const
{
    return m_liveWaveform;
}

QString MemoAudioEngine::playingMemoId() const
{
    return m_playingMemoId;
}

double MemoAudioEngine::playbackPositionSec() const
{
    return m_playbackPositionSec;
}

double MemoAudioEngine::playbackDurationSec() const
{
    return m_playbackDurationSec;
}

bool MemoAudioEngine::playbackPaused() const
{
    return m_playbackPaused;
}

double MemoAudioEngine::playbackSpeed() const
{
    return m_playbackSpeed;
}

bool MemoAudioEngine::skipSilence() const
{
    return m_skipSilence;
}

bool MemoAudioEngine::enhanceRecording() const
{
    return m_enhanceRecording;
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
    m_recordingElapsedMs = 0;
    m_currentLevel = 0.06;
    m_liveWaveform.clear();
    m_liveWaveform.append(0.08);
    m_recordTimer->start();

    Q_EMIT recordingChanged(m_recording);
    Q_EMIT recordingSecondsChanged(m_recordingSeconds);
    Q_EMIT recordingElapsedMsChanged(m_recordingElapsedMs);
    Q_EMIT currentLevelChanged(m_currentLevel);
    Q_EMIT liveWaveformChanged();
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

void MemoAudioEngine::playMemo(const QString &id, const QString &filePath, const QString &title, double startSec)
{
    if (startSec < 0.0 && m_playingMemoId == id && !m_playbackPaused && m_playProcess && m_playProcess->state() != QProcess::NotRunning) {
        stopPlayback();
        return;
    }

    const double clampedStart = std::max(0.0, startSec);
    startPlaybackProcess(id, filePath, title, clampedStart);
}

void MemoAudioEngine::togglePlayPause(const QString &id, const QString &filePath, const QString &title)
{
    if (m_playingMemoId == id && !m_playbackPaused && m_playProcess && m_playProcess->state() != QProcess::NotRunning) {
        killPlaybackProcess();
        m_playTimer->stop();
        m_playbackPaused = true;
        Q_EMIT playbackPausedChanged(true);
        Q_EMIT statusRequested(QStringLiteral("En pausa: %1").arg(title));
        return;
    }

    const double resumeAt = (m_playingMemoId == id && m_playbackPaused) ? m_playbackPositionSec : 0.0;
    startPlaybackProcess(id, filePath, title, resumeAt);
}

void MemoAudioEngine::seekPlayback(double seconds, const QString &id, const QString &filePath, const QString &title)
{
    const double total = std::max<double>(computeWavDurationSeconds(filePath), 1.0);
    const double clamped = qBound(0.0, seconds, total);

    const bool wasPlayingSame = (m_playingMemoId == id && !m_playbackPaused && m_playProcess && m_playProcess->state() != QProcess::NotRunning);

    if (m_playingMemoId != id) {
        stopPlayback();
        m_playingMemoId = id;
        m_playingFilePath = filePath;
        m_playingTitle = title;
        m_playbackDurationSec = total;
        m_playbackPaused = true;
        Q_EMIT playingMemoIdChanged(m_playingMemoId);
        Q_EMIT playbackDurationSecChanged(m_playbackDurationSec);
        Q_EMIT playbackPausedChanged(true);
    }

    m_playbackPositionSec = clamped;
    Q_EMIT playbackPositionSecChanged(m_playbackPositionSec);

    if (wasPlayingSame) {
        startPlaybackProcess(id, filePath, title, clamped);
    }
}

void MemoAudioEngine::skipPlayback(double deltaSeconds, const QString &id, const QString &filePath, const QString &title)
{
    const double base = (m_playingMemoId == id) ? m_playbackPositionSec : 0.0;
    seekPlayback(base + deltaSeconds, id, filePath, title);
}

void MemoAudioEngine::setPlaybackSpeed(double speed)
{
    const double clamped = qBound(0.5, speed, 2.0);
    if (qFuzzyCompare(m_playbackSpeed, clamped)) {
        return;
    }
    m_playbackSpeed = clamped;
    Q_EMIT playbackSpeedChanged(m_playbackSpeed);

    if (!m_playingMemoId.isEmpty() && !m_playbackPaused && m_playProcess) {
        startPlaybackProcess(m_playingMemoId, m_playingFilePath, m_playingTitle, m_playbackPositionSec);
    }
}

void MemoAudioEngine::setSkipSilence(bool enabled)
{
    if (m_skipSilence == enabled) {
        return;
    }
    m_skipSilence = enabled;
    Q_EMIT skipSilenceChanged(m_skipSilence);

    if (!m_playingMemoId.isEmpty() && !m_playbackPaused && m_playProcess) {
        startPlaybackProcess(m_playingMemoId, m_playingFilePath, m_playingTitle, m_playbackPositionSec);
    }
}

void MemoAudioEngine::setEnhanceRecording(bool enabled)
{
    if (m_enhanceRecording == enabled) {
        return;
    }
    m_enhanceRecording = enabled;
    Q_EMIT enhanceRecordingChanged(m_enhanceRecording);

    if (!m_playingMemoId.isEmpty() && !m_playbackPaused && m_playProcess) {
        startPlaybackProcess(m_playingMemoId, m_playingFilePath, m_playingTitle, m_playbackPositionSec);
    }
}

void MemoAudioEngine::startPlaybackProcess(const QString &id, const QString &filePath, const QString &title, double startSec)
{
    killPlaybackProcess();
    m_playTimer->stop();

    if (!QFileInfo::exists(filePath)) {
        Q_EMIT statusRequested(QStringLiteral("Archivo WAV no encontrado: %1").arg(filePath));
        return;
    }

    const double total = std::max<double>(computeWavDurationSeconds(filePath), 1.0);
    const double clampedStart = qBound(0.0, startSec, std::max(0.0, total - 0.1));

    const QString ffplay = QStandardPaths::findExecutable(QStringLiteral("ffplay"));
    const QString pwPlay = QStandardPaths::findExecutable(QStringLiteral("pw-play"));
    if (ffplay.isEmpty() && pwPlay.isEmpty()) {
        Q_EMIT statusRequested(QStringLiteral("Error: no se encontró ffplay ni pw-play en el sistema."));
        return;
    }

    m_playingMemoId = id;
    m_playingFilePath = filePath;
    m_playingTitle = title;
    m_playbackDurationSec = total;
    m_playbackPositionSec = clampedStart;
    m_playbackPaused = false;

    Q_EMIT playingMemoIdChanged(m_playingMemoId);
    Q_EMIT playbackDurationSecChanged(m_playbackDurationSec);
    Q_EMIT playbackPositionSecChanged(m_playbackPositionSec);
    Q_EMIT playbackPausedChanged(false);

    m_playProcess = new QProcess(this);
    connect(m_playProcess, &QProcess::finished, this, [this, id](int, QProcess::ExitStatus) {
        m_playTimer->stop();
        if (m_playingMemoId == id && !m_playbackPaused) {
            m_playingMemoId.clear();
            m_playbackPositionSec = 0.0;
            Q_EMIT playbackPositionSecChanged(0.0);
            Q_EMIT playingMemoIdChanged(m_playingMemoId);
        }
        if (m_playProcess) {
            m_playProcess->deleteLater();
            m_playProcess = nullptr;
        }
    });

    if (!ffplay.isEmpty()) {
        QStringList args;
        args << QStringLiteral("-nodisp")
             << QStringLiteral("-autoexit")
             << QStringLiteral("-loglevel") << QStringLiteral("quiet");
        if (clampedStart > 0.05) {
            args << QStringLiteral("-ss") << QString::number(clampedStart, 'f', 2);
        }
        QStringList filters;
        if (m_enhanceRecording) {
            filters << QStringLiteral("highpass=f=85")
                    << QStringLiteral("lowpass=f=7600")
                    << QStringLiteral("afftdn=nf=-22");
        }
        if (m_skipSilence) {
            filters << QStringLiteral("silenceremove=start_periods=1:stop_periods=-1:stop_duration=0.45:stop_threshold=-38dB");
        }
        if (!qFuzzyCompare(m_playbackSpeed, 1.0)) {
            filters << QStringLiteral("atempo=%1").arg(m_playbackSpeed, 0, 'f', 2);
        }
        if (!filters.isEmpty()) {
            args << QStringLiteral("-af") << filters.join(QLatin1Char(','));
        }
        args << filePath;
        m_playProcess->start(ffplay, args);
    } else {
        m_playProcess->start(pwPlay, {filePath});
    }

    m_playTimer->start();
    Q_EMIT statusRequested(QStringLiteral("Reproduciendo: %1").arg(title));
}

void MemoAudioEngine::killPlaybackProcess()
{
    if (m_playProcess) {
        disconnect(m_playProcess, nullptr, this, nullptr);
        if (m_playProcess->state() != QProcess::NotRunning) {
            m_playProcess->terminate();
            if (!m_playProcess->waitForFinished(400)) {
                m_playProcess->kill();
            }
        }
        m_playProcess->deleteLater();
        m_playProcess = nullptr;
    }
}

void MemoAudioEngine::stopPlayback()
{
    m_playTimer->stop();
    killPlaybackProcess();

    const bool hadId = !m_playingMemoId.isEmpty();
    m_playingMemoId.clear();
    m_playingFilePath.clear();
    m_playingTitle.clear();
    m_playbackPositionSec = 0.0;
    m_playbackDurationSec = 0.0;
    if (m_playbackPaused) {
        m_playbackPaused = false;
        Q_EMIT playbackPausedChanged(false);
    }
    Q_EMIT playbackPositionSecChanged(0.0);
    Q_EMIT playbackDurationSecChanged(0.0);
    if (hadId) {
        Q_EMIT playingMemoIdChanged(m_playingMemoId);
    }
}

int MemoAudioEngine::computeWavDurationSeconds(const QString &wavPath)
{
    QFile f(wavPath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return 0;
    }
    const WavHeaderInfo info = parseWavHeader(f);
    f.close();
    if (!info.valid || info.byteRate == 0) {
        return 0;
    }
    return static_cast<int>(info.dataSize / info.byteRate);
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

QVariantList MemoAudioEngine::extractWaveformPeaks(const QString &wavPath, int barCount)
{
    const int count = std::clamp(barCount, 16, 400);
    QVariantList peaks;
    peaks.reserve(count);

    QFile f(wavPath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        for (int i = 0; i < count; ++i) {
            peaks.append(0.08);
        }
        return peaks;
    }

    const WavHeaderInfo info = parseWavHeader(f);
    if (!info.valid || info.bitsPerSample != 16 || info.dataSize < 2) {
        f.close();
        for (int i = 0; i < count; ++i) {
            peaks.append(0.08);
        }
        return peaks;
    }

    const qint64 totalFrames = info.dataSize / std::max<quint16>(2, info.blockAlign);
    const qint64 framesPerBucket = std::max<qint64>(1, totalFrames / count);
    const int bytesPerFrame = std::max<int>(2, info.blockAlign);

    for (int b = 0; b < count; ++b) {
        const qint64 startFrame = (b * totalFrames) / count;
        const qint64 endFrame = std::min(totalFrames, ((b + 1) * totalFrames) / count);
        const qint64 bucketFrames = std::max<qint64>(1, endFrame - startFrame);
        const qint64 stepFrames = std::max<qint64>(1, bucketFrames / 64);

        double maxSample = 0.0;
        double sumSq = 0.0;
        int sampled = 0;

        for (qint64 fr = startFrame; fr < startFrame + std::min<qint64>(bucketFrames, framesPerBucket); fr += stepFrames) {
            f.seek(info.dataOffset + fr * bytesPerFrame);
            qint16 s = 0;
            if (f.read(reinterpret_cast<char *>(&s), sizeof(qint16)) != sizeof(qint16)) {
                break;
            }
            const double amp = std::abs(static_cast<double>(s)) / 32768.0;
            if (amp > maxSample) {
                maxSample = amp;
            }
            sumSq += amp * amp;
            ++sampled;
        }

        const double rms = sampled > 0 ? std::sqrt(sumSq / sampled) : 0.0;
        const double level = qBound(0.06, (rms * 0.65 + maxSample * 0.35) * 2.8, 1.0);
        peaks.append(level);
    }

    f.close();
    return peaks;
}

bool MemoAudioEngine::trimWavFile(const QString &srcPath, const QString &destPath, double startSec, double endSec)
{
    if (!QFileInfo::exists(srcPath)) {
        return false;
    }

    QFile inFile(srcPath);
    if (!inFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    const WavHeaderInfo info = parseWavHeader(inFile);
    if (!info.valid || info.byteRate == 0 || info.blockAlign == 0) {
        inFile.close();
        return false;
    }

    const double totalSec = static_cast<double>(info.dataSize) / static_cast<double>(info.byteRate);
    const double s0 = qBound(0.0, startSec, totalSec);
    const double s1 = qBound(s0, endSec, totalSec);
    if (s1 - s0 < 0.2) {
        inFile.close();
        return false;
    }

    qint64 startByte = static_cast<qint64>(s0 * info.byteRate);
    qint64 endByte = static_cast<qint64>(s1 * info.byteRate);
    startByte = (startByte / info.blockAlign) * info.blockAlign;
    endByte = (endByte / info.blockAlign) * info.blockAlign;
    endByte = std::min(info.dataSize, std::max(startByte + info.blockAlign, endByte));

    const qint64 slicedBytes = endByte - startByte;
    inFile.seek(info.dataOffset + startByte);
    const QByteArray pcm = inFile.read(slicedBytes);
    inFile.close();
    if (pcm.isEmpty()) {
        return false;
    }

    const bool sameFile = (QFileInfo(srcPath).canonicalFilePath() == QFileInfo(destPath).canonicalFilePath() && !destPath.isEmpty());
    const QString writePath = sameFile ? (destPath + QStringLiteral(".trimtmp")) : destPath;

    QFile outFile(writePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    char header[44] = {};
    const qint32 dataSize32 = static_cast<qint32>(pcm.size());
    const qint32 chunkSize32 = 36 + dataSize32;
    const qint32 subchunk1Size = 16;
    const qint16 audioFormat = 1;
    const qint16 numChannels = static_cast<qint16>(info.channels);
    const qint32 sampleRate = static_cast<qint32>(info.sampleRate);
    const qint32 byteRate = static_cast<qint32>(info.byteRate);
    const qint16 blockAlign = static_cast<qint16>(info.blockAlign);
    const qint16 bitsPerSample = static_cast<qint16>(info.bitsPerSample);

    std::memcpy(header, "RIFF", 4);
    std::memcpy(header + 4, &chunkSize32, 4);
    std::memcpy(header + 8, "WAVE", 4);
    std::memcpy(header + 12, "fmt ", 4);
    std::memcpy(header + 16, &subchunk1Size, 4);
    std::memcpy(header + 20, &audioFormat, 2);
    std::memcpy(header + 22, &numChannels, 2);
    std::memcpy(header + 24, &sampleRate, 4);
    std::memcpy(header + 28, &byteRate, 4);
    std::memcpy(header + 32, &blockAlign, 2);
    std::memcpy(header + 34, &bitsPerSample, 2);
    std::memcpy(header + 36, "data", 4);
    std::memcpy(header + 40, &dataSize32, 4);

    outFile.write(header, 44);
    outFile.write(pcm);
    outFile.close();

    if (sameFile) {
        QFile::remove(destPath);
        if (!QFile::rename(writePath, destPath)) {
            QFile::remove(writePath);
            return false;
        }
    }
    return true;
}

void MemoAudioEngine::updateRecordingMetrics()
{
    if (!m_recording) {
        return;
    }

    m_recordingElapsedMs += 100;
    Q_EMIT recordingElapsedMsChanged(m_recordingElapsedMs);

    if (m_recordingElapsedMs % 1000 == 0) {
        ++m_recordingSeconds;
        Q_EMIT recordingSecondsChanged(m_recordingSeconds);
    }

    QFile f(m_activeRecordingPath);
    if (f.open(QIODevice::ReadOnly)) {
        const qint64 sz = f.size();
        if (sz > 44) {
            const qint64 bytesToRead = std::min<qint64>(sz - 44, 3200);
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
                m_currentLevel = qBound(0.05, rms * 6.0, 1.0);
                Q_EMIT currentLevelChanged(m_currentLevel);
            }
        }
        f.close();
    }

    m_liveWaveform.append(m_currentLevel);
    if (m_liveWaveform.size() > 240) {
        m_liveWaveform.removeFirst();
    }
    Q_EMIT liveWaveformChanged();
}

void MemoAudioEngine::updatePlaybackMetrics()
{
    if (m_playingMemoId.isEmpty() || m_playbackPaused) {
        return;
    }
    m_playbackPositionSec = std::min(m_playbackDurationSec, m_playbackPositionSec + (0.05 * m_playbackSpeed));
    Q_EMIT playbackPositionSecChanged(m_playbackPositionSec);
}
