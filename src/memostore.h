/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class MemoStore : public QObject
{
    Q_OBJECT

public:
    explicit MemoStore(const QString &storageDir = QString(),
                       const QString &knowledgeDir = QString(),
                       QObject *parent = nullptr);
    ~MemoStore() override = default;

    QVariantList memos() const;
    QVariantList folders() const;
    QString selectedFolder() const;
    QString currentFolderTitle() const;
    QString sortBy() const;
    QString searchQuery() const;
    QString selectedMemoId() const;
    QVariantMap selectedMemo() const;

    int allCount() const;
    int favoritesCount() const;
    int transcribedCount() const;
    int trashCount() const;

    QString storageDir() const;
    void setStorageDir(const QString &dir);

    QString knowledgeDir() const;
    void setKnowledgeDir(const QString &dir);

    bool loadMemos();
    bool saveMemos() const;

    void selectMemo(const QString &id);
    void setSelectedFolder(const QString &folder);
    void setSortBy(const QString &sortBy);
    void filterMemos(const QString &searchQuery);
    int findMemoIndex(const QString &id) const;

    void updateMemoField(const QString &id, const QString &key, const QVariant &value, bool persist = true);

    void addMemo(const QString &id, const QString &title, const QString &filePath, int durationSec);
    QString addMemoFromWav(const QString &wavPath,
                           const QString &title = QString(),
                           const QString &transcript = QString(),
                           const QString &summary = QString());
    QString importWavFile(const QString &fileUrl);
    void renameMemo(const QString &id, const QString &newTitle);
    void deleteMemo(const QString &id);

    void toggleFavorite(const QString &id);
    void moveToTrash(const QString &id);
    void restoreMemo(const QString &id);
    void emptyTrash();
    QString duplicateMemo(const QString &id);
    bool trimMemo(const QString &id, double startSec, double endSec, bool saveAsNew);

    bool createFolder(const QString &name);
    bool renameFolder(const QString &oldName, const QString &newName);
    bool deleteFolder(const QString &name);
    void moveMemoToFolder(const QString &id, const QString &folder);

    bool exportToKityKnowledge(const QString &id);
    bool exportAudioCopy(const QString &id, const QString &destUrl);
    static QString slugifyTitle(const QString &title);

Q_SIGNALS:
    void memosChanged();
    void foldersChanged();
    void selectedFolderChanged();
    void sortByChanged();
    void searchQueryChanged();
    void selectedMemoIdChanged();
    void selectedMemoChanged();
    void memoDeleted(const QString &id);
    void statusRequested(const QString &msg);

private:
    void loadFolders();
    void saveFolders() const;
    static bool isReservedFolderId(const QString &id);
    void ensureValidSelectionAfterFilter();

    QString m_storageDir;
    QString m_knowledgeDir;
    QString m_selectedFolder = QStringLiteral("all");
    QString m_sortBy = QStringLiteral("date");
    QString m_searchQuery;
    QStringList m_customFolderNames;
    QVariantList m_allMemos;
    QString m_selectedMemoId;
};
