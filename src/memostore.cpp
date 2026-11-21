/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "memostore.h"
#include "memoaudioengine.h"

#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

MemoStore::MemoStore(const QString &storageDir,
                     const QString &knowledgeDir,
                     QObject *parent)
    : QObject(parent)
{
    const QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    m_storageDir = storageDir.isEmpty()
        ? (genericData + QStringLiteral("/kvoicememos"))
        : storageDir;
    m_knowledgeDir = knowledgeDir.isEmpty()
        ? (genericData + QStringLiteral("/kity/knowledge"))
        : knowledgeDir;

    QDir().mkpath(m_storageDir);
    loadFolders();
    loadMemos();
    if (!m_allMemos.isEmpty()) {
        const QVariantList visible = memos();
        if (!visible.isEmpty()) {
            m_selectedMemoId = visible.first().toMap().value(QStringLiteral("id")).toString();
        } else {
            m_selectedMemoId = m_allMemos.first().toMap().value(QStringLiteral("id")).toString();
        }
    }
}

bool MemoStore::isReservedFolderId(const QString &id)
{
    const QString lower = id.trimmed().toLower();
    return lower == QLatin1String("all")
        || lower == QLatin1String("favorites")
        || lower == QLatin1String("transcribed")
        || lower == QLatin1String("trash");
}

QVariantList MemoStore::memos() const
{
    const bool trashView = (m_selectedFolder == QLatin1String("trash"));
    const QString q = m_searchQuery.trimmed().toLower();

    QVariantList result;
    for (const QVariant &v : m_allMemos) {
        const QVariantMap map = v.toMap();
        const bool isDeleted = map.value(QStringLiteral("deleted")).toBool();
        if (trashView != isDeleted) {
            continue;
        }

        if (!trashView) {
            if (m_selectedFolder == QLatin1String("favorites")
                && !map.value(QStringLiteral("favorite")).toBool()) {
                continue;
            }
            if (m_selectedFolder == QLatin1String("transcribed")
                && map.value(QStringLiteral("transcript")).toString().trimmed().isEmpty()) {
                continue;
            }
            if (m_selectedFolder != QLatin1String("all")
                && m_selectedFolder != QLatin1String("favorites")
                && m_selectedFolder != QLatin1String("transcribed")
                && map.value(QStringLiteral("folder")).toString() != m_selectedFolder) {
                continue;
            }
        }

        if (!q.isEmpty()) {
            const QString title = map.value(QStringLiteral("title")).toString().toLower();
            const QString transcript = map.value(QStringLiteral("transcript")).toString().toLower();
            const QString summary = map.value(QStringLiteral("summary")).toString().toLower();
            if (!title.contains(q) && !transcript.contains(q) && !summary.contains(q)) {
                continue;
            }
        }

        result.append(map);
    }

    if (m_sortBy == QLatin1String("title")) {
        std::stable_sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
            return a.toMap().value(QStringLiteral("title")).toString().compare(
                       b.toMap().value(QStringLiteral("title")).toString(), Qt::CaseInsensitive) < 0;
        });
    } else if (m_sortBy == QLatin1String("duration")) {
        std::stable_sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
            return a.toMap().value(QStringLiteral("durationSec")).toInt()
                > b.toMap().value(QStringLiteral("durationSec")).toInt();
        });
    }

    return result;
}

QVariantList MemoStore::folders() const
{
    QVariantList list;
    for (const QString &name : m_customFolderNames) {
        int count = 0;
        for (const QVariant &v : m_allMemos) {
            const QVariantMap map = v.toMap();
            if (!map.value(QStringLiteral("deleted")).toBool()
                && map.value(QStringLiteral("folder")).toString() == name) {
                ++count;
            }
        }
        QVariantMap item;
        item.insert(QStringLiteral("name"), name);
        item.insert(QStringLiteral("count"), count);
        list.append(item);
    }
    return list;
}

QString MemoStore::selectedFolder() const
{
    return m_selectedFolder;
}

QString MemoStore::currentFolderTitle() const
{
    if (m_selectedFolder == QLatin1String("all")) {
        return QStringLiteral("Todas las grabaciones");
    }
    if (m_selectedFolder == QLatin1String("favorites")) {
        return QStringLiteral("Favoritos");
    }
    if (m_selectedFolder == QLatin1String("transcribed")) {
        return QStringLiteral("Con transcripción");
    }
    if (m_selectedFolder == QLatin1String("trash")) {
        return QStringLiteral("Eliminadas");
    }
    return m_selectedFolder;
}

QString MemoStore::sortBy() const
{
    return m_sortBy;
}

QString MemoStore::searchQuery() const
{
    return m_searchQuery;
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

int MemoStore::allCount() const
{
    int count = 0;
    for (const QVariant &v : m_allMemos) {
        if (!v.toMap().value(QStringLiteral("deleted")).toBool()) {
            ++count;
        }
    }
    return count;
}

int MemoStore::favoritesCount() const
{
    int count = 0;
    for (const QVariant &v : m_allMemos) {
        const QVariantMap m = v.toMap();
        if (!m.value(QStringLiteral("deleted")).toBool() && m.value(QStringLiteral("favorite")).toBool()) {
            ++count;
        }
    }
    return count;
}

int MemoStore::transcribedCount() const
{
    int count = 0;
    for (const QVariant &v : m_allMemos) {
        const QVariantMap m = v.toMap();
        if (!m.value(QStringLiteral("deleted")).toBool()
            && !m.value(QStringLiteral("transcript")).toString().trimmed().isEmpty()) {
            ++count;
        }
    }
    return count;
}

int MemoStore::trashCount() const
{
    int count = 0;
    for (const QVariant &v : m_allMemos) {
        if (v.toMap().value(QStringLiteral("deleted")).toBool()) {
            ++count;
        }
    }
    return count;
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
    loadFolders();
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

void MemoStore::loadFolders()
{
    m_customFolderNames.clear();
    const QString foldersPath = m_storageDir + QStringLiteral("/folders.json");
    QFile f(foldersPath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isArray()) {
        return;
    }
    for (const QJsonValue &val : doc.array()) {
        const QString name = val.toString().trimmed();
        if (!name.isEmpty() && !isReservedFolderId(name) && !m_customFolderNames.contains(name)) {
            m_customFolderNames.append(name);
        }
    }
    Q_EMIT foldersChanged();
}

void MemoStore::saveFolders() const
{
    QDir().mkpath(m_storageDir);
    const QString foldersPath = m_storageDir + QStringLiteral("/folders.json");
    QFile f(foldersPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    QJsonArray arr;
    for (const QString &name : m_customFolderNames) {
        arr.append(name);
    }
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    f.close();
}

bool MemoStore::loadMemos()
{
    m_allMemos.clear();
    const QString jsonPath = m_storageDir + QStringLiteral("/memos.json");
    QFile f(jsonPath);
    if (!f.exists()) {
        Q_EMIT memosChanged();
        Q_EMIT foldersChanged();
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
        if (!map.contains(QStringLiteral("favorite"))) {
            map[QStringLiteral("favorite")] = false;
        }
        if (!map.contains(QStringLiteral("folder"))) {
            map[QStringLiteral("folder")] = QString();
        }
        if (!map.contains(QStringLiteral("deleted"))) {
            map[QStringLiteral("deleted")] = false;
        }
        const QString filePath = map.value(QStringLiteral("filePath")).toString();
        map[QStringLiteral("waveform")] = MemoAudioEngine::extractWaveformPeaks(filePath);
        m_allMemos.append(map);
    }
    Q_EMIT memosChanged();
    Q_EMIT foldersChanged();
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
        map.remove(QStringLiteral("waveform"));
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

void MemoStore::setSelectedFolder(const QString &folder)
{
    const QString normalized = folder.trimmed().isEmpty() ? QStringLiteral("all") : folder.trimmed();
    if (m_selectedFolder == normalized) {
        return;
    }
    m_selectedFolder = normalized;
    Q_EMIT selectedFolderChanged();
    Q_EMIT memosChanged();
    ensureValidSelectionAfterFilter();
}

void MemoStore::setSortBy(const QString &sortBy)
{
    QString normalized = sortBy.trimmed();
    if (normalized != QLatin1String("date")
        && normalized != QLatin1String("title")
        && normalized != QLatin1String("duration")) {
        normalized = QStringLiteral("date");
    }
    if (m_sortBy == normalized) {
        return;
    }
    m_sortBy = normalized;
    Q_EMIT sortByChanged();
    Q_EMIT memosChanged();
}

void MemoStore::filterMemos(const QString &searchQuery)
{
    if (m_searchQuery == searchQuery) {
        return;
    }
    m_searchQuery = searchQuery;
    Q_EMIT searchQueryChanged();
    Q_EMIT memosChanged();
}

void MemoStore::ensureValidSelectionAfterFilter()
{
    const QVariantList visible = memos();
    for (const QVariant &v : visible) {
        if (v.toMap().value(QStringLiteral("id")).toString() == m_selectedMemoId) {
            return;
        }
    }
    m_selectedMemoId = visible.isEmpty()
        ? QString()
        : visible.first().toMap().value(QStringLiteral("id")).toString();
    Q_EMIT selectedMemoIdChanged();
    Q_EMIT selectedMemoChanged();
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
    Q_EMIT foldersChanged();
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
    memo[QStringLiteral("favorite")] = (m_selectedFolder == QLatin1String("favorites"));
    memo[QStringLiteral("folder")] = (!isReservedFolderId(m_selectedFolder) && m_customFolderNames.contains(m_selectedFolder))
        ? m_selectedFolder
        : QString();
    memo[QStringLiteral("deleted")] = false;
    memo[QStringLiteral("transcribing")] = false;
    memo[QStringLiteral("summarizing")] = false;
    memo[QStringLiteral("waveform")] = MemoAudioEngine::extractWaveformPeaks(filePath);

    if (m_selectedFolder == QLatin1String("trash")) {
        m_selectedFolder = QStringLiteral("all");
        Q_EMIT selectedFolderChanged();
    }

    m_allMemos.prepend(memo);
    saveMemos();
    m_selectedMemoId = id;
    Q_EMIT memosChanged();
    Q_EMIT foldersChanged();
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
    memo[QStringLiteral("favorite")] = false;
    memo[QStringLiteral("folder")] = QString();
    memo[QStringLiteral("deleted")] = false;
    memo[QStringLiteral("transcribing")] = false;
    memo[QStringLiteral("summarizing")] = false;
    memo[QStringLiteral("waveform")] = MemoAudioEngine::extractWaveformPeaks(wavPath);

    m_allMemos.prepend(memo);
    saveMemos();
    m_selectedMemoId = id;
    Q_EMIT memosChanged();
    Q_EMIT foldersChanged();
    Q_EMIT selectedMemoIdChanged();
    Q_EMIT selectedMemoChanged();
    return id;
}

QString MemoStore::importWavFile(const QString &fileUrl)
{
    QString localPath = fileUrl.trimmed();
    if (localPath.startsWith(QLatin1String("file://"))) {
        localPath = QUrl(localPath).toLocalFile();
    }
    if (localPath.isEmpty() || !QFileInfo::exists(localPath)) {
        Q_EMIT statusRequested(QStringLiteral("Archivo WAV no encontrado."));
        return QString();
    }

    QDir().mkpath(m_storageDir);
    const QString baseTitle = QFileInfo(localPath).completeBaseName();
    const QDateTime now = QDateTime::currentDateTime();
    const QString destPath = m_storageDir + QStringLiteral("/memo_%1_%2.wav")
                                                .arg(now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")),
                                                     QUuid::createUuid().toString(QUuid::Id128).left(4));
    if (!QFile::copy(localPath, destPath)) {
        Q_EMIT statusRequested(QStringLiteral("No se pudo copiar el archivo WAV al almacén."));
        return QString();
    }

    const QString newId = addMemoFromWav(destPath, baseTitle);
    Q_EMIT statusRequested(QStringLiteral("Audio importado: «%1».").arg(baseTitle));
    return newId;
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

    Q_EMIT memosChanged();
    Q_EMIT foldersChanged();

    if (m_selectedMemoId == id) {
        ensureValidSelectionAfterFilter();
    }
    Q_EMIT statusRequested(QStringLiteral("Nota de voz eliminada."));
}

void MemoStore::toggleFavorite(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const bool next = !m_allMemos.at(idx).toMap().value(QStringLiteral("favorite")).toBool();
    updateMemoField(id, QStringLiteral("favorite"), next, true);
    Q_EMIT statusRequested(next ? QStringLiteral("Añadida a Favoritos.")
                                : QStringLiteral("Quitada de Favoritos."));
}

void MemoStore::moveToTrash(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    if (m_allMemos.at(idx).toMap().value(QStringLiteral("deleted")).toBool()) {
        deleteMemo(id);
        return;
    }

    Q_EMIT memoDeleted(id);
    updateMemoField(id, QStringLiteral("deleted"), true, true);
    ensureValidSelectionAfterFilter();
    Q_EMIT statusRequested(QStringLiteral("Nota movida a Eliminadas."));
}

void MemoStore::restoreMemo(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    updateMemoField(id, QStringLiteral("deleted"), false, true);
    if (m_selectedFolder == QLatin1String("trash")) {
        ensureValidSelectionAfterFilter();
    }
    Q_EMIT statusRequested(QStringLiteral("Nota restaurada."));
}

void MemoStore::emptyTrash()
{
    QStringList idsToDelete;
    for (const QVariant &v : m_allMemos) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("deleted")).toBool()) {
            idsToDelete.append(m.value(QStringLiteral("id")).toString());
        }
    }
    for (const QString &id : idsToDelete) {
        deleteMemo(id);
    }
    Q_EMIT statusRequested(QStringLiteral("Papelera de grabaciones vaciada."));
}

QString MemoStore::duplicateMemo(const QString &id)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return QString();
    }
    const QVariantMap src = m_allMemos.at(idx).toMap();
    const QString srcWav = src.value(QStringLiteral("filePath")).toString();
    if (!QFileInfo::exists(srcWav)) {
        Q_EMIT statusRequested(QStringLiteral("No se encontró el archivo WAV original para duplicar."));
        return QString();
    }

    QDir().mkpath(m_storageDir);
    const QDateTime now = QDateTime::currentDateTime();
    const QString newId = now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")) + QStringLiteral("_")
        + QUuid::createUuid().toString(QUuid::Id128).left(4);
    const QString destWav = m_storageDir + QStringLiteral("/memo_%1.wav").arg(newId);
    if (!QFile::copy(srcWav, destWav)) {
        Q_EMIT statusRequested(QStringLiteral("Error al copiar el archivo de audio."));
        return QString();
    }

    QVariantMap copy = src;
    copy[QStringLiteral("id")] = newId;
    copy[QStringLiteral("title")] = QString(src.value(QStringLiteral("title")).toString() + QStringLiteral(" (copia)"));
    copy[QStringLiteral("filePath")] = destWav;
    copy[QStringLiteral("createdAt")] = now.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    copy[QStringLiteral("deleted")] = false;
    copy[QStringLiteral("transcribing")] = false;
    copy[QStringLiteral("summarizing")] = false;
    copy[QStringLiteral("waveform")] = MemoAudioEngine::extractWaveformPeaks(destWav);

    m_allMemos.prepend(copy);
    saveMemos();
    m_selectedMemoId = newId;
    Q_EMIT memosChanged();
    Q_EMIT foldersChanged();
    Q_EMIT selectedMemoIdChanged();
    Q_EMIT selectedMemoChanged();
    Q_EMIT statusRequested(QStringLiteral("Nota de voz duplicada."));
    return newId;
}

bool MemoStore::trimMemo(const QString &id, double startSec, double endSec, bool saveAsNew)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return false;
    }

    Q_EMIT memoDeleted(id);

    const QVariantMap src = m_allMemos.at(idx).toMap();
    const QString srcWav = src.value(QStringLiteral("filePath")).toString();

    if (saveAsNew) {
        QDir().mkpath(m_storageDir);
        const QDateTime now = QDateTime::currentDateTime();
        const QString newId = now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")) + QStringLiteral("_")
            + QUuid::createUuid().toString(QUuid::Id128).left(4);
        const QString destWav = m_storageDir + QStringLiteral("/memo_%1.wav").arg(newId);
        if (!MemoAudioEngine::trimWavFile(srcWav, destWav, startSec, endSec)) {
            Q_EMIT statusRequested(QStringLiteral("No se pudo recortar el archivo de audio."));
            return false;
        }
        const QString newTitle = src.value(QStringLiteral("title")).toString() + QStringLiteral(" (recorte)");
        addMemo(newId, newTitle, destWav, std::max(1, MemoAudioEngine::computeWavDurationSeconds(destWav)));
        Q_EMIT statusRequested(QStringLiteral("Recorte guardado como nueva grabación."));
        return true;
    }

    if (!MemoAudioEngine::trimWavFile(srcWav, srcWav, startSec, endSec)) {
        Q_EMIT statusRequested(QStringLiteral("No se pudo recortar el archivo de audio."));
        return false;
    }

    QVariantMap updated = src;
    updated[QStringLiteral("durationSec")] = std::max(1, MemoAudioEngine::computeWavDurationSeconds(srcWav));
    updated[QStringLiteral("waveform")] = MemoAudioEngine::extractWaveformPeaks(srcWav);
    m_allMemos[idx] = updated;
    saveMemos();
    Q_EMIT memosChanged();
    if (m_selectedMemoId == id) {
        Q_EMIT selectedMemoChanged();
    }
    Q_EMIT statusRequested(QStringLiteral("Grabación recortada correctamente."));
    return true;
}

bool MemoStore::createFolder(const QString &name)
{
    const QString cleaned = name.trimmed();
    if (cleaned.isEmpty() || isReservedFolderId(cleaned) || m_customFolderNames.contains(cleaned)) {
        return false;
    }
    m_customFolderNames.append(cleaned);
    saveFolders();
    Q_EMIT foldersChanged();
    Q_EMIT statusRequested(QStringLiteral("Carpeta «%1» creada.").arg(cleaned));
    return true;
}

bool MemoStore::renameFolder(const QString &oldName, const QString &newName)
{
    const QString oldCleaned = oldName.trimmed();
    const QString newCleaned = newName.trimmed();
    const int idx = m_customFolderNames.indexOf(oldCleaned);
    if (idx < 0 || newCleaned.isEmpty() || isReservedFolderId(newCleaned)
        || (oldCleaned != newCleaned && m_customFolderNames.contains(newCleaned))) {
        return false;
    }

    m_customFolderNames[idx] = newCleaned;
    for (int i = 0; i < m_allMemos.size(); ++i) {
        QVariantMap map = m_allMemos.at(i).toMap();
        if (map.value(QStringLiteral("folder")).toString() == oldCleaned) {
            map[QStringLiteral("folder")] = newCleaned;
            m_allMemos[i] = map;
        }
    }
    saveFolders();
    saveMemos();
    if (m_selectedFolder == oldCleaned) {
        m_selectedFolder = newCleaned;
        Q_EMIT selectedFolderChanged();
    }
    Q_EMIT foldersChanged();
    Q_EMIT memosChanged();
    Q_EMIT selectedMemoChanged();
    Q_EMIT statusRequested(QStringLiteral("Carpeta renombrada a «%1».").arg(newCleaned));
    return true;
}

bool MemoStore::deleteFolder(const QString &name)
{
    const QString cleaned = name.trimmed();
    if (!m_customFolderNames.removeOne(cleaned)) {
        return false;
    }
    for (int i = 0; i < m_allMemos.size(); ++i) {
        QVariantMap map = m_allMemos.at(i).toMap();
        if (map.value(QStringLiteral("folder")).toString() == cleaned) {
            map[QStringLiteral("folder")] = QString();
            m_allMemos[i] = map;
        }
    }
    saveFolders();
    saveMemos();
    if (m_selectedFolder == cleaned) {
        m_selectedFolder = QStringLiteral("all");
        Q_EMIT selectedFolderChanged();
    }
    Q_EMIT foldersChanged();
    Q_EMIT memosChanged();
    Q_EMIT selectedMemoChanged();
    Q_EMIT statusRequested(QStringLiteral("Carpeta «%1» eliminada.").arg(cleaned));
    return true;
}

void MemoStore::moveMemoToFolder(const QString &id, const QString &folder)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return;
    }
    const QString dest = folder.trimmed();
    if (!dest.isEmpty() && !isReservedFolderId(dest) && !m_customFolderNames.contains(dest)) {
        m_customFolderNames.append(dest);
        saveFolders();
    }
    QVariantMap map = m_allMemos.at(idx).toMap();
    map[QStringLiteral("folder")] = dest;
    map[QStringLiteral("deleted")] = false;
    m_allMemos[idx] = map;
    saveMemos();
    Q_EMIT memosChanged();
    Q_EMIT foldersChanged();
    if (m_selectedMemoId == id) {
        Q_EMIT selectedMemoChanged();
    }
    Q_EMIT statusRequested(dest.isEmpty()
                               ? QStringLiteral("Movida a Todas las grabaciones.")
                               : QStringLiteral("Movida a la carpeta «%1».").arg(dest));
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

    const QString slug = QString(slugifyTitle(title) + QStringLiteral("-") + id);
    const QString outPath = QString(m_knowledgeDir + QStringLiteral("/") + slug + QStringLiteral(".md"));

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

bool MemoStore::exportAudioCopy(const QString &id, const QString &destUrl)
{
    const int idx = findMemoIndex(id);
    if (idx < 0) {
        return false;
    }
    const QString srcWav = m_allMemos.at(idx).toMap().value(QStringLiteral("filePath")).toString();
    if (!QFileInfo::exists(srcWav)) {
        Q_EMIT statusRequested(QStringLiteral("No se encontró el archivo WAV."));
        return false;
    }
    QString localPath = destUrl.trimmed();
    if (localPath.startsWith(QLatin1String("file://"))) {
        localPath = QUrl(localPath).toLocalFile();
    }
    if (localPath.isEmpty()) {
        return false;
    }
    if (!localPath.endsWith(QLatin1String(".wav"), Qt::CaseInsensitive)) {
        localPath += QStringLiteral(".wav");
    }
    if (QFileInfo::exists(localPath)) {
        QFile::remove(localPath);
    }
    if (!QFile::copy(srcWav, localPath)) {
        Q_EMIT statusRequested(QStringLiteral("Error al exportar copia de audio."));
        return false;
    }
    Q_EMIT statusRequested(QStringLiteral("Audio exportado: %1").arg(QFileInfo(localPath).fileName()));
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
