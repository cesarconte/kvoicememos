/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QObject>
#include <QString>
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
    QString selectedMemoId() const;
    QVariantMap selectedMemo() const;

    QString storageDir() const;
    void setStorageDir(const QString &dir);

    QString knowledgeDir() const;
    void setKnowledgeDir(const QString &dir);

    bool loadMemos();
    bool saveMemos() const;

    void selectMemo(const QString &id);
    void filterMemos(const QString &searchQuery);
    int findMemoIndex(const QString &id) const;

    void updateMemoField(const QString &id, const QString &key, const QVariant &value, bool persist = true);

    void addMemo(const QString &id, const QString &title, const QString &filePath, int durationSec);
    QString addMemoFromWav(const QString &wavPath,
                           const QString &title = QString(),
                           const QString &transcript = QString(),
                           const QString &summary = QString());
    void renameMemo(const QString &id, const QString &newTitle);
    void deleteMemo(const QString &id);

    bool exportToKityKnowledge(const QString &id);
    static QString slugifyTitle(const QString &title);

Q_SIGNALS:
    void memosChanged();
    void selectedMemoIdChanged();
    void selectedMemoChanged();
    void memoDeleted(const QString &id);
    void statusRequested(const QString &msg);

private:
    QString m_storageDir;
    QString m_knowledgeDir;
    QString m_searchQuery;
    QVariantList m_allMemos;
    QString m_selectedMemoId;
};
