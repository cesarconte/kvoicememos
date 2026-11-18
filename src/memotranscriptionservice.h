/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class MemoTranscriptionService : public QObject
{
    Q_OBJECT

public:
    explicit MemoTranscriptionService(QObject *parent = nullptr);
    ~MemoTranscriptionService() override = default;

    void transcribe(const QString &id, const QString &wavPath);
    void summarize(const QString &id, const QString &transcript);

    static QString cleanWhisperOutput(const QString &raw);
    static QString resolveWhisperBinary();
    static QString resolveWhisperModel();

Q_SIGNALS:
    void transcriptionStarted(const QString &id);
    void transcriptionFinished(const QString &id, const QString &transcript, bool success);
    void summaryStarted(const QString &id);
    void summaryFinished(const QString &id, const QString &summary, bool success);
    void statusRequested(const QString &msg);

private:
    QNetworkAccessManager *m_nam = nullptr;
};
