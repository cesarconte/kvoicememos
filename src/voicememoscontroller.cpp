/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "voicememoscontroller.h"

#include <cmath>
#include <csignal>
#include <cstring>

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

VoiceMemosController::VoiceMemosController(const QString &storageDir,
                                           const QString &knowledgeDir,
                                           QObject *parent)
    : QObject(parent)
    , m_storageDir(storageDir.isEmpty()
                       ? (QDir::homePath() + QStringLiteral("/.local/share/kvoicememos"))
                       : storageDir)
    , m_knowledgeDir(knowledgeDir.isEmpty()
                         ? (QDir::homePath() + QStringLiteral("/.local/share/kiri/knowledge"))
                         : knowledgeDir)
    , m_recordTimer(new QTimer(this))
    , m_nam(new QNetworkAccessManager(this))
{
    QDir().mkpath(m_storageDir);
    m_recordTimer->setInterval(200);
    connect(m_recordTimer, &QTimer::timeout, this, &VoiceMemosController::updateRecordingMetrics);
    loadMemos();
    if (!m_allMemos.isEmpty()) {
        m_selectedMemoId = m_allMemos.first().toMap().value(QStringLiteral("id")).toString();
    }
}

VoiceMemosController::~VoiceMemosController()
{
    if (m_recording) {
        stopRecording();
    }
    stopPlayback();
}

QVariantList VoiceMemosController::memos() const
{
    if (m_searchQuery.trimmed().isEmpty()) {
        return m_allMemos;
    }
    const QString q = m_searchQuery.trimmed().toLower();
    QVariantList filtered;
    for (const QVariant &v : m_allMemos) {
        const QVariantMap map = v.toMap();
        const QString title = map.value(QStringLiteral("title")).toString().toLower();
        const QString transcript = map.value(QStringLiteral("transcript")).toString().toLower();
        const QString summary = map.value(QStringLiteral("summary")).toString().toLower();
        if (title.contains(q) || transcript.contains(q) || summary.contains(q)) {
            filtered.append(map);
        }
    }
    return filtered;
}

bool VoiceMemosController::recording() const
{
    return m_recording;
}

int VoiceMemosController::recordingSeconds() const
{
    return m_recordingSeconds;
}

double VoiceMemosController::currentLevel() const
{
    return m_currentLevel;
}

QString VoiceMemosController::playingMemoId() const
{
    return m_playingMemoId;
}

QString VoiceMemosController::selectedMemoId() const
{
    return m_selectedMemoId;
}

QVariantMap VoiceMemosController::selectedMemo() const
{
    const int idx = findMemoIndex(m_selectedMemoId);
    if (idx < 0) {
        return QVariantMap();
    }
    return m_allMemos.at(idx).toMap();
}

QString VoiceMemosController::statusMessage() const
{
    return m_statusMessage;
}

QString VoiceMemosController::storageDir() const
{
    return m_storageDir;
}

void VoiceMemosController::setStorageDir(const QString &dir)
{
    if (dir.isEmpty() || m_storageDir == dir) {
        return;
    }
    m_storageDir = dir;
    QDir().mkpath(m_storageDir);
    loadMemos();
}

QString VoiceMemosController::knowledgeDir() const
{
    return m_knowledgeDir;
}

void VoiceMemosController::setKnowledgeDir(const QString &dir)
{
    if (!dir.isEmpty()) {
        m_knowledgeDir = dir;
    }
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
    QString text = raw;
    // Remove timestamp blocks like [00:00:00.000 --> 00:00:03.000]
    static const QRegularExpression tsRx(
        QStringLiteral("\\[\\d{2}:\\d{2}(?::\\d{2})?\\.\\d{3}\\s*-->\\s*\\d{2}:\\d{2}(?::\\d{2})?\\.\\d{3}\\]"));
    text.remove(tsRx);

    // Remove [BLANK_AUDIO], [Música], (applause), etc.
    static const QRegularExpression bracketRx(QStringLiteral("\\[[^\\]]*\\]|\\([^\\)]*\\)"));
    text.remove(bracketRx);

    return text.simplified().trimmed();
}

int VoiceMemosController::computeWavDurationSeconds(const QString &wavPath)
{
    QFileInfo fi(wavPath);
    if (!fi.exists() || fi.size() <= 44) {
        return 0;
    }
    const qint64 pcmBytes = fi.size() - 44;
    // 16000 Hz * 1 channel * 2 bytes (s16le) = 32000 bytes/sec
    return static_cast<int>(pcmBytes / (16000 * 2));
}

QByteArray VoiceMemosController::createSyntheticWav(int durationSeconds, int sampleRate)
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

QString VoiceMemosController::slugifyTitle(const QString &title)
{
    QString slug = title.trimmed().toLower();
    slug.replace(QStringLiteral("á"), QStringLiteral("a"));
    slug.replace(QStringLiteral("é"), QStringLiteral("e"));
    slug.replace(QStringLiteral("í"), QStringLiteral("i"));
    slug.replace(QStringLiteral("ó"), QStringLiteral("o"));
    slug.replace(QStringLiteral("ú"), QStringLiteral("u"));
    slug.replace(QStringLiteral("ñ"), QStringLiteral("n"));
    slug.replace(QStringLiteral("ü"), QStringLiteral("u"));
    static const QRegularExpression nonAlnum(QStringLiteral("[^a-z0-9]+"));
    slug.replace(nonAlnum, QStringLiteral("-"));
    slug.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
    if (slug.isEmpty()) {
        slug = QStringLiteral("nota-de-voz");
    }
    return slug;
}

void VoiceMemosController::startRecording(const QString &customTitle)
{
    if (m_recording) {
        return;
    }

    stopPlayback();

    const QString pwRecord = QStandardPaths::findExecutable(QStringLiteral("pw-record"));
    if (pwRecord.isEmpty()) {
        setStatus(QStringLiteral("Error: no se encontró pw-record (PipeWire) en el sistema."));
        return;
    }

    QDir().mkpath(m_storageDir);
    const QDateTime now = QDateTime::currentDateTime();
    m_activeRecordingId = now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    m_activeRecordingTitle = customTitle.trimmed().isEmpty()
        ? QStringLiteral("Nota de voz %1").arg(now.toString(QStringLiteral("dd/MM HH:mm")))
        : customTitle.trimmed();
    m_activeRecordingPath = m_storageDir + QStringLiteral("/memo_%1.wav").arg(m_activeRecordingId);

    m_recordProcess = new QProcess(this);
    QStringList args;
    args << QStringLiteral("--format=s16")
         << QStringLiteral("--rate=16000")
         << QStringLiteral("--channels=1")
         << m_activeRecordingPath;

    m_recordProcess->start(pwRecord, args);
    if (!m_recordProcess->waitForStarted(2000)) {
        setStatus(QStringLiteral("No se pudo iniciar la grabación con PipeWire."));
        m_recordProcess->deleteLater();
        m_recordProcess = nullptr;
        return;
    }

    m_recording = true;
    m_recordingSeconds = 0;
    m_recordingTickCount = 0;
    m_currentLevel = 0.05;
    m_recordTimer->start();

    Q_EMIT recordingChanged();
    Q_EMIT recordingSecondsChanged();
    Q_EMIT currentLevelChanged();
    setStatus(QStringLiteral("Grabando desde PipeWire (16 kHz mono)…"));
}

void VoiceMemosController::stopRecording()
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
    Q_EMIT recordingChanged();
    Q_EMIT currentLevelChanged();

    QFileInfo fi(m_activeRecordingPath);
    if (!fi.exists() || fi.size() <= 44) {
        QFile::remove(m_activeRecordingPath);
        setStatus(QStringLiteral("Grabación descartada (archivo de audio vacío)."));
        return;
    }

    const int dur = std::max(1, computeWavDurationSeconds(m_activeRecordingPath));
    QVariantMap memo;
    memo[QStringLiteral("id")] = m_activeRecordingId;
    memo[QStringLiteral("title")] = m_activeRecordingTitle;
    memo[QStringLiteral("filePath")] = m_activeRecordingPath;
    memo[QStringLiteral("createdAt")] = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    memo[QStringLiteral("durationSec")] = dur;
    memo[QStringLiteral("transcript")] = QString();
    memo[QStringLiteral("summary")] = QString();
    memo[QStringLiteral("transcribing")] = false;
    memo[QStringLiteral("summarizing")] = false;

    m_allMemos.prepend(memo);
    saveMemos();
    m_selectedMemoId = m_activeRecordingId;
    Q_EMIT memosChanged();
    Q_EMIT selectedMemoIdChanged();
    Q_EMIT selectedMemoChanged();

    setStatus(QStringLiteral("Grabación guardada (%1 s).").arg(dur));

    if (m_autoTranscribeOnStop) {
        transcribeMemo(m_activeRecordingId);
    }
}

void VoiceMemosController::playMemo(const QString &id)
{
    if (m_playingMemoId == id && m_playProcess && m_playProcess->state() != QProcess::NotRunning) {
        stopPlayback();
        return;
    }

    stopPlayback();

    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const QVariantMap memo = m_allMemos.at(idx).toMap();
    const QString filePath = memo.value(QStringLiteral("filePath")).toString();
    if (!QFileInfo::exists(filePath)) {
        setStatus(QStringLiteral("Archivo WAV no encontrado: %1").arg(filePath));
        return;
    }

    const QString pwPlay = QStandardPaths::findExecutable(QStringLiteral("pw-play"));
    if (pwPlay.isEmpty()) {
        setStatus(QStringLiteral("Error: no se encontró pw-play en el sistema."));
        return;
    }

    m_playProcess = new QProcess(this);
    m_playingMemoId = id;
    Q_EMIT playingMemoIdChanged();

    connect(m_playProcess, &QProcess::finished, this, [this, id](int, QProcess::ExitStatus) {
        if (m_playingMemoId == id) {
            m_playingMemoId.clear();
            Q_EMIT playingMemoIdChanged();
        }
        if (m_playProcess) {
            m_playProcess->deleteLater();
            m_playProcess = nullptr;
        }
    });

    m_playProcess->start(pwPlay, {filePath});
    setStatus(QStringLiteral("Reproduciendo: %1").arg(memo.value(QStringLiteral("title")).toString()));
}

void VoiceMemosController::stopPlayback()
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
        Q_EMIT playingMemoIdChanged();
    }
}

void VoiceMemosController::transcribeMemo(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const QVariantMap memo = m_allMemos.at(idx).toMap();
    const QString wavPath = memo.value(QStringLiteral("filePath")).toString();
    if (!QFileInfo::exists(wavPath)) {
        setStatus(QStringLiteral("No existe el archivo WAV para transcribir."));
        return;
    }

    const QString whisperBin = resolveWhisperBinary();
    const QString modelPath = resolveWhisperModel();
    if (whisperBin.isEmpty() || modelPath.isEmpty()) {
        setStatus(QStringLiteral("Whisper-cli o el modelo ggml-base.bin no están disponibles."));
        return;
    }

    updateMemoField(id, QStringLiteral("transcribing"), true, false);
    setStatus(QStringLiteral("Transcribiendo con Whisper.cpp…"));

    QProcess *proc = new QProcess(this);
    QStringList args;
    args << QStringLiteral("-m") << modelPath
         << QStringLiteral("-l") << QStringLiteral("es")
         << QStringLiteral("-nt")
         << QStringLiteral("-f") << wavPath;

    connect(proc, &QProcess::finished, this, [this, proc, id](int exitCode, QProcess::ExitStatus) {
        const QString rawOut = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        updateMemoField(id, QStringLiteral("transcribing"), false, false);

        if (exitCode != 0) {
            setStatus(QStringLiteral("Error al ejecutar whisper-cli (código %1).").arg(exitCode));
            return;
        }

        const QString cleaned = cleanWhisperOutput(rawOut);
        if (cleaned.isEmpty()) {
            updateMemoField(id, QStringLiteral("transcript"), QStringLiteral("(Sin voz detectada en la grabación)"), true);
            setStatus(QStringLiteral("Transcripción finalizada (sin voz detectada)."));
            return;
        }

        updateMemoField(id, QStringLiteral("transcript"), cleaned, true);
        setStatus(QStringLiteral("Transcripción completada. Generando resumen IA…"));
        summarizeMemo(id);
    });

    proc->start(whisperBin, args);
}

void VoiceMemosController::summarizeMemo(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const QVariantMap memo = m_allMemos.at(idx).toMap();
    const QString transcript = memo.value(QStringLiteral("transcript")).toString().trimmed();
    if (transcript.isEmpty() || transcript.startsWith(QStringLiteral("(Sin voz"))) {
        setStatus(QStringLiteral("No hay transcripción válida para resumir."));
        return;
    }

    updateMemoField(id, QStringLiteral("summarizing"), true, false);
    setStatus(QStringLiteral("Resumiendo con Ollama (qwen2.5:7b)…"));

    QJsonObject reqObj;
    reqObj[QStringLiteral("model")] = QStringLiteral("qwen2.5:7b");
    reqObj[QStringLiteral("stream")] = false;

    QJsonArray msgs;
    QJsonObject sys;
    sys[QStringLiteral("role")] = QStringLiteral("system");
    sys[QStringLiteral("content")] = QStringLiteral(
        "Eres el asistente de Notas de Voz de KDE (estilo Apple Intelligence). "
        "A partir de la transcripción, responde en español con:\n"
        "1. **Título sugerido**: (máximo 6 palabras)\n"
        "2. **Puntos clave**: (lista concisa con viñetas de las ideas o tareas principales).");
    msgs.append(sys);

    QJsonObject usr;
    usr[QStringLiteral("role")] = QStringLiteral("user");
    usr[QStringLiteral("content")] = transcript;
    msgs.append(usr);

    reqObj[QStringLiteral("messages")] = msgs;

    QNetworkRequest req(QUrl(QStringLiteral("http://127.0.0.1:11434/api/chat")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam->post(req, QJsonDocument(reqObj).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();
        updateMemoField(id, QStringLiteral("summarizing"), false, false);

        if (reply->error() != QNetworkReply::NoError) {
            setStatus(QStringLiteral("No se pudo conectar con Ollama (127.0.0.1:11434)."));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QString summary = doc.object()
                                    .value(QStringLiteral("message"))
                                    .toObject()
                                    .value(QStringLiteral("content"))
                                    .toString()
                                    .trimmed();
        if (summary.isEmpty()) {
            setStatus(QStringLiteral("Ollama devolvió una respuesta vacía."));
            return;
        }

        updateMemoField(id, QStringLiteral("summary"), summary, true);
        setStatus(QStringLiteral("Resumen IA generado con éxito."));
    });
}

void VoiceMemosController::renameMemo(const QString &id, const QString &newTitle)
{
    const QString trimmed = newTitle.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    updateMemoField(id, QStringLiteral("title"), trimmed, true);
    setStatus(QStringLiteral("Nota renombrada a «%1».").arg(trimmed));
}

void VoiceMemosController::deleteMemo(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    if (m_playingMemoId == id) {
        stopPlayback();
    }

    const QVariantMap memo = m_allMemos.at(idx).toMap();
    const QString wavPath = memo.value(QStringLiteral("filePath")).toString();
    if (!wavPath.isEmpty() && QFileInfo::exists(wavPath)) {
        QFile::remove(wavPath);
    }

    m_allMemos.removeAt(idx);
    saveMemos();

    if (m_selectedMemoId == id) {
        m_selectedMemoId = m_allMemos.isEmpty()
            ? QString()
            : m_allMemos.first().toMap().value(QStringLiteral("id")).toString();
        Q_EMIT selectedMemoIdChanged();
        Q_EMIT selectedMemoChanged();
    }

    Q_EMIT memosChanged();
    setStatus(QStringLiteral("Nota de voz eliminada."));
}

bool VoiceMemosController::exportToKiriKnowledge(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        setStatus(QStringLiteral("Selecciona una nota de voz para exportar."));
        return false;
    }

    const QVariantMap memo = m_allMemos.at(idx).toMap();
    const QString title = memo.value(QStringLiteral("title")).toString();
    const QString createdAt = memo.value(QStringLiteral("createdAt")).toString();
    const int durationSec = memo.value(QStringLiteral("durationSec")).toInt();
    const QString summary = memo.value(QStringLiteral("summary")).toString();
    const QString transcript = memo.value(QStringLiteral("transcript")).toString();

    if (!QDir().mkpath(m_knowledgeDir)) {
        setStatus(QStringLiteral("No se pudo crear el directorio de Kiri RAG."));
        return false;
    }

    const QString slug = slugifyTitle(title) + QStringLiteral("-") + id;
    const QString outPath = m_knowledgeDir + QStringLiteral("/") + slug + QStringLiteral(".md");

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("No se pudo escribir el archivo en Kiri RAG."));
        return false;
    }

    QString md;
    md += QStringLiteral("# %1\n\n").arg(title);
    md += QStringLiteral("- **Fecha**: %1\n").arg(createdAt);
    md += QStringLiteral("- **Duración**: %1 s\n").arg(durationSec);
    md += QStringLiteral("- **Origen**: KVoiceMemos\n\n");
    if (!summary.isEmpty()) {
        md += QStringLiteral("## Resumen IA\n\n%1\n\n").arg(summary);
    }
    md += QStringLiteral("## Transcripción completa\n\n%1\n").arg(
        transcript.isEmpty() ? QStringLiteral("_(Sin transcripción)_") : transcript);

    f.write(md.toUtf8());
    f.close();

    setStatus(QStringLiteral("Exportado a Kiri Knowledge: %1.md").arg(slug));
    return true;
}

void VoiceMemosController::selectMemo(const QString &id)
{
    if (m_selectedMemoId == id) {
        return;
    }
    if (findMemoIndex(id) < 0 && !id.isEmpty()) {
        return;
    }
    m_selectedMemoId = id;
    Q_EMIT selectedMemoIdChanged();
    Q_EMIT selectedMemoChanged();
}

void VoiceMemosController::filterMemos(const QString &searchQuery)
{
    if (m_searchQuery == searchQuery) {
        return;
    }
    m_searchQuery = searchQuery;
    Q_EMIT memosChanged();
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
    const QDateTime now = QDateTime::currentDateTime();
    const QString id = now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")) + QStringLiteral("_")
        + QUuid::createUuid().toString(QUuid::Id128).left(4);

    const int dur = computeWavDurationSeconds(wavPath);
    QVariantMap memo;
    memo[QStringLiteral("id")] = id;
    memo[QStringLiteral("title")] = title.trimmed().isEmpty() ? QStringLiteral("Nota importada") : title.trimmed();
    memo[QStringLiteral("filePath")] = wavPath;
    memo[QStringLiteral("createdAt")] = now.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    memo[QStringLiteral("durationSec")] = dur;
    memo[QStringLiteral("transcript")] = transcript;
    memo[QStringLiteral("summary")] = summary;
    memo[QStringLiteral("transcribing")] = false;
    memo[QStringLiteral("summarizing")] = false;

    m_allMemos.prepend(memo);
    saveMemos();
    m_selectedMemoId = id;
    Q_EMIT memosChanged();
    Q_EMIT selectedMemoIdChanged();
    Q_EMIT selectedMemoChanged();
    return id;
}

bool VoiceMemosController::loadMemos()
{
    m_allMemos.clear();
    const QString jsonPath = m_storageDir + QStringLiteral("/memos.json");
    QFile f(jsonPath);
    if (!f.exists()) {
        Q_EMIT memosChanged();
        return true;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isArray()) {
        return false;
    }
    const QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        if (!val.isObject()) {
            continue;
        }
        QVariantMap map = val.toObject().toVariantMap();
        map[QStringLiteral("transcribing")] = false;
        map[QStringLiteral("summarizing")] = false;
        m_allMemos.append(map);
    }
    Q_EMIT memosChanged();
    Q_EMIT selectedMemoChanged();
    return true;
}

bool VoiceMemosController::saveMemos() const
{
    QDir().mkpath(m_storageDir);
    const QString jsonPath = m_storageDir + QStringLiteral("/memos.json");
    QFile f(jsonPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QJsonArray arr;
    for (const QVariant &v : m_allMemos) {
        QVariantMap map = v.toMap();
        map.remove(QStringLiteral("transcribing"));
        map.remove(QStringLiteral("summarizing"));
        arr.append(QJsonObject::fromVariantMap(map));
    }
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

void VoiceMemosController::updateRecordingMetrics()
{
    if (!m_recording) {
        return;
    }

    ++m_recordingTickCount;
    if (m_recordingTickCount % 5 == 0) {
        ++m_recordingSeconds;
        Q_EMIT recordingSecondsChanged();
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
                Q_EMIT currentLevelChanged();
            }
        }
        f.close();
    }
}

void VoiceMemosController::setStatus(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        Q_EMIT statusMessageChanged();
    }
}

int VoiceMemosController::findMemoIndex(const QString &id) const
{
    if (id.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_allMemos.size(); ++i) {
        if (m_allMemos.at(i).toMap().value(QStringLiteral("id")).toString() == id) {
            return i;
        }
    }
    return -1;
}

void VoiceMemosController::updateMemoField(const QString &id,
                                           const QString &key,
                                           const QVariant &value,
                                           bool persist)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    QVariantMap map = m_allMemos.at(idx).toMap();
    map[key] = value;
    m_allMemos[idx] = map;
    if (persist) {
        saveMemos();
    }
    Q_EMIT memosChanged();
    if (m_selectedMemoId == id) {
        Q_EMIT selectedMemoChanged();
    }
}

QString VoiceMemosController::resolveWhisperBinary() const
{
    const QString localBin = QDir::homePath() + QStringLiteral("/.local/bin/whisper-cli");
    if (QFileInfo::exists(localBin)) {
        return localBin;
    }
    const QString sysBin = QStandardPaths::findExecutable(QStringLiteral("whisper-cli"));
    if (!sysBin.isEmpty()) {
        return sysBin;
    }
    return QStandardPaths::findExecutable(QStringLiteral("whisper"));
}

QString VoiceMemosController::resolveWhisperModel() const
{
    const QString model = QDir::homePath() + QStringLiteral("/.local/share/kiri/models/ggml-base.bin");
    if (QFileInfo::exists(model)) {
        return model;
    }
    return QString();
}
