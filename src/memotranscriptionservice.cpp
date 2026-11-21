/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "memotranscriptionservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

MemoTranscriptionService::MemoTranscriptionService(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

QString MemoTranscriptionService::cleanWhisperOutput(const QString &raw)
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

QString MemoTranscriptionService::resolveWhisperBinary()
{
    const QString sysBin = QStandardPaths::findExecutable(QStringLiteral("whisper-cli"));
    if (!sysBin.isEmpty()) {
        return sysBin;
    }
    return QStandardPaths::findExecutable(QStringLiteral("whisper"));
}

QString MemoTranscriptionService::resolveWhisperModel()
{
    for (const QString &rel : {
             QStringLiteral("kity/models/ggml-base.bin"),
             QStringLiteral("kiri/models/ggml-base.bin"),
         }) {
        const QString located = QStandardPaths::locate(QStandardPaths::GenericDataLocation, rel);
        if (!located.isEmpty()) {
            return located;
        }
    }
    const QString sysModel = QStringLiteral("/usr/share/kity/models/ggml-base.bin");
    if (QFileInfo::exists(sysModel)) {
        return sysModel;
    }
    return QString();
}

void MemoTranscriptionService::transcribe(const QString &id, const QString &wavPath)
{
    if (!QFileInfo::exists(wavPath)) {
        Q_EMIT statusRequested(QStringLiteral("No existe el archivo WAV para transcribir."));
        return;
    }

    const QString whisperBin = resolveWhisperBinary();
    const QString modelPath = resolveWhisperModel();
    if (whisperBin.isEmpty() || modelPath.isEmpty()) {
        Q_EMIT statusRequested(QStringLiteral("Whisper-cli o el modelo ggml-base.bin no están disponibles."));
        return;
    }

    Q_EMIT transcriptionStarted(id);
    Q_EMIT statusRequested(QStringLiteral("Transcribiendo con Whisper.cpp…"));

    QProcess *proc = new QProcess(this);
    QStringList args;
    args << QStringLiteral("-m") << modelPath
         << QStringLiteral("-l") << QStringLiteral("es")
         << QStringLiteral("-nt")
         << QStringLiteral("-f") << wavPath;

    connect(proc, &QProcess::finished, this, [this, proc, id](int exitCode, QProcess::ExitStatus) {
        const QString rawOut = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        if (exitCode != 0) {
            Q_EMIT statusRequested(QStringLiteral("Error al ejecutar whisper-cli (código %1).").arg(exitCode));
            Q_EMIT transcriptionFinished(id, QString(), false);
            return;
        }

        const QString cleaned = cleanWhisperOutput(rawOut);
        if (cleaned.isEmpty()) {
            Q_EMIT transcriptionFinished(id, QStringLiteral("(Sin voz detectada en la grabación)"), true);
            Q_EMIT statusRequested(QStringLiteral("Transcripción finalizada (sin voz detectada)."));
            return;
        }

        Q_EMIT transcriptionFinished(id, cleaned, true);
        Q_EMIT statusRequested(QStringLiteral("Transcripción completada. Generando resumen IA…"));
        summarize(id, cleaned);
    });

    proc->start(whisperBin, args);
}

void MemoTranscriptionService::summarize(const QString &id, const QString &transcript)
{
    const QString trimmed = transcript.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("(Sin voz"))) {
        Q_EMIT statusRequested(QStringLiteral("No hay transcripción válida para resumir."));
        return;
    }

    Q_EMIT summaryStarted(id);
    Q_EMIT statusRequested(QStringLiteral("Resumiendo con Ollama (qwen2.5:7b)…"));

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
    usr[QStringLiteral("content")] = trimmed;
    msgs.append(usr);

    reqObj[QStringLiteral("messages")] = msgs;

    QNetworkRequest req(QUrl(QStringLiteral("http://127.0.0.1:11434/api/chat")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam->post(req, QJsonDocument(reqObj).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT statusRequested(QStringLiteral("No se pudo conectar con Ollama (127.0.0.1:11434)."));
            Q_EMIT summaryFinished(id, QString(), false);
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
            Q_EMIT statusRequested(QStringLiteral("Ollama devolvió una respuesta vacía."));
            Q_EMIT summaryFinished(id, QString(), false);
            return;
        }

        Q_EMIT summaryFinished(id, summary, true);
        Q_EMIT statusRequested(QStringLiteral("Resumen IA generado con éxito."));
    });
}
