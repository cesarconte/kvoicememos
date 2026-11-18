/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "memostore.h"
#include "memoaudioengine.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

MemoStore::MemoStore(const QString &storageDir,
                     const QString &knowledgeDir,
                     QObject *parent)
    : QObject(parent)
    , m_storageDir(storageDir.isEmpty()
                       ? (QDir::homePath() + QStringLiteral("/.local/share/kvoicememos"))
                       : storageDir)
    , m_knowledgeDir(knowledgeDir.isEmpty()
                         ? (QDir::homePath() + QStringLiteral("/.local/share/kity/knowledge"))
                         : knowledgeDir)
{
    QDir().mkpath(m_storageDir);
    loadMemos();
    if (!m_allMemos.isEmpty()) {
        m_selectedMemoId = m_allMemos.first().toMap().value(QStringLiteral("id")).toString();
    }
}

QVariantList MemoStore::memos() const
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

QString MemoStore::selectedMemoId() const
{
    return m_selectedMemoId;
}

QVariantMap MemoStore::selectedMemo() const
{
    const int idx = findMemoIndex(m_selectedMemoId);
    if (idx < 0) {
        return QVariantMap();
    }
    return m_allMemos.at(idx).toMap();
}

QString MemoStore::storageDir() const
{
    return m_storageDir;
}

void MemoStore::setStorageDir(const QString &dir)
{
    if (dir.isEmpty() || m_storageDir == dir) {
        return;
    }
    m_storageDir = dir;
    QDir().mkpath(m_storageDir);
    loadMemos();
}

QString MemoStore::knowledgeDir() const
{
    return m_knowledgeDir;
}

void MemoStore::setKnowledgeDir(const QString &dir)
{
    if (!dir.isEmpty()) {
        m_knowledgeDir = dir;
    }
}

bool MemoStore::loadMemos()
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

bool MemoStore::saveMemos() const
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

void MemoStore::selectMemo(const QString &id)
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

void MemoStore::filterMemos(const QString &searchQuery)
{
    if (m_searchQuery == searchQuery) {
        return;
    }
    m_searchQuery = searchQuery;
    Q_EMIT memosChanged();
}

int MemoStore::findMemoIndex(const QString &id) const
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

void MemoStore::updateMemoField(const QString &id, const QString &key, const QVariant &value, bool persist)
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

void MemoStore::addMemo(const QString &id, const QString &title, const QString &filePath, int durationSec)
{
    QVariantMap memo;
    memo[QStringLiteral("id")] = id;
    memo[QStringLiteral("title")] = title;
    memo[QStringLiteral("filePath")] = filePath;
    memo[QStringLiteral("createdAt")] = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    memo[QStringLiteral("durationSec")] = durationSec;
    memo[QStringLiteral("transcript")] = QString();
    memo[QStringLiteral("summary")] = QString();
    memo[QStringLiteral("transcribing")] = false;
    memo[QStringLiteral("summarizing")] = false;

    m_allMemos.prepend(memo);
    saveMemos();
    m_selectedMemoId = id;
    Q_EMIT memosChanged();
    Q_EMIT selectedMemoIdChanged();
    Q_EMIT selectedMemoChanged();
}

QString MemoStore::addMemoFromWav(const QString &wavPath,
                                  const QString &title,
                                  const QString &transcript,
                                  const QString &summary)
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString id = now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")) + QStringLiteral("_")
        + QUuid::createUuid().toString(QUuid::Id128).left(4);

    const int dur = MemoAudioEngine::computeWavDurationSeconds(wavPath);
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

void MemoStore::renameMemo(const QString &id, const QString &newTitle)
{
    const QString trimmed = newTitle.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    updateMemoField(id, QStringLiteral("title"), trimmed, true);
    Q_EMIT statusRequested(QStringLiteral("Nota renombrada a «%1».").arg(trimmed));
}

void MemoStore::deleteMemo(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }

    Q_EMIT memoDeleted(id);

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
    Q_EMIT statusRequested(QStringLiteral("Nota de voz eliminada."));
}

bool MemoStore::exportToKityKnowledge(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        Q_EMIT statusRequested(QStringLiteral("Selecciona una nota de voz para exportar."));
        return false;
    }

    const QVariantMap memo = m_allMemos.at(idx).toMap();
    const QString title = memo.value(QStringLiteral("title")).toString();
    const QString createdAt = memo.value(QStringLiteral("createdAt")).toString();
    const int durationSec = memo.value(QStringLiteral("durationSec")).toInt();
    const QString summary = memo.value(QStringLiteral("summary")).toString();
    const QString transcript = memo.value(QStringLiteral("transcript")).toString();

    if (!QDir().mkpath(m_knowledgeDir)) {
        Q_EMIT statusRequested(QStringLiteral("No se pudo crear el directorio de Kity RAG."));
        return false;
    }

    const QString slug = slugifyTitle(title) + QStringLiteral("-") + id;
    const QString outPath = m_knowledgeDir + QStringLiteral("/") + slug + QStringLiteral(".md");

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Q_EMIT statusRequested(QStringLiteral("No se pudo escribir el archivo en Kity RAG."));
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

    Q_EMIT statusRequested(QStringLiteral("Exportado a Kity Knowledge: %1.md").arg(slug));
    return true;
}

QString MemoStore::slugifyTitle(const QString &title)
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
