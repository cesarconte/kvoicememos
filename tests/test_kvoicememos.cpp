/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "voicememoscontroller.h"

#include <functional>

#include <QDir>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>

class TestKVoiceMemos : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(resources);
    }

    void testCleanWhisperOutput()
    {
        const QString raw = QStringLiteral(
            "[00:00:00.000 --> 00:00:02.400]   Hola mundo desde PipeWire.  \n"
            "[00:00:02.400 --> 00:00:04.000] [BLANK_AUDIO] (música) Reunión de arquitectura mañana.");
        const QString cleaned = VoiceMemosController::cleanWhisperOutput(raw);
        QCOMPARE(cleaned, QStringLiteral("Hola mundo desde PipeWire. Reunión de arquitectura mañana."));
    }

    void testWavDurationCalculation()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString wavPath = tmpDir.filePath(QStringLiteral("synthetic_7s.wav"));
        QFile f(wavPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(VoiceMemosController::createSyntheticWav(7, 16000));
        f.close();

        QCOMPARE(VoiceMemosController::computeWavDurationSeconds(wavPath), 7);
    }

    void testPersistenceRenameFilterAndDelete()
    {
        QTemporaryDir storageTmp;
        QTemporaryDir knowledgeTmp;
        QVERIFY(storageTmp.isValid());
        QVERIFY(knowledgeTmp.isValid());

        const QString wav1 = storageTmp.filePath(QStringLiteral("memo1.wav"));
        {
            QFile f(wav1);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(VoiceMemosController::createSyntheticWav(4, 16000));
            f.close();
        }

        const QString wav2 = storageTmp.filePath(QStringLiteral("memo2.wav"));
        {
            QFile f(wav2);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(VoiceMemosController::createSyntheticWav(9, 16000));
            f.close();
        }

        QString id1;
        QString id2;
        {
            VoiceMemosController ctrl(storageTmp.path(), knowledgeTmp.path());
            QCOMPARE(ctrl.memos().size(), 0);

            id1 = ctrl.addMemoFromWav(
                wav1,
                QStringLiteral("Idea de proyecto"),
                QStringLiteral("Implementar integración con Kiri RAG"),
                QStringLiteral("- Integrar Kiri RAG"));
            id2 = ctrl.addMemoFromWav(
                wav2,
                QStringLiteral("Lista de compras"),
                QStringLiteral("Comprar café y teclado mecánico"),
                QStringLiteral("- Café\n- Teclado"));

            QCOMPARE(ctrl.memos().size(), 2);

            // Rename first memo
            ctrl.renameMemo(id1, QStringLiteral("Arquitectura Kiri 2.0"));
            ctrl.selectMemo(id1);
            QCOMPARE(ctrl.selectedMemo().value(QStringLiteral("title")).toString(),
                     QStringLiteral("Arquitectura Kiri 2.0"));

            // Filter by transcript/summary/title
            ctrl.filterMemos(QStringLiteral("teclado"));
            QCOMPARE(ctrl.memos().size(), 1);
            QCOMPARE(ctrl.memos().first().toMap().value(QStringLiteral("id")).toString(), id2);

            ctrl.filterMemos(QString());
            QCOMPARE(ctrl.memos().size(), 2);
        }

        // Reload in a fresh controller instance to verify memos.json persistence
        {
            VoiceMemosController reloaded(storageTmp.path(), knowledgeTmp.path());
            QCOMPARE(reloaded.memos().size(), 2);
            reloaded.selectMemo(id1);
            QCOMPARE(reloaded.selectedMemo().value(QStringLiteral("title")).toString(),
                     QStringLiteral("Arquitectura Kiri 2.0"));
            QCOMPARE(reloaded.selectedMemo().value(QStringLiteral("durationSec")).toInt(), 4);

            // Delete second memo and verify WAV removal
            QVERIFY(QFile::exists(wav2));
            reloaded.deleteMemo(id2);
            QCOMPARE(reloaded.memos().size(), 1);
            QVERIFY(!QFile::exists(wav2));
        }
    }

    void testExportToKiriKnowledge()
    {
        QTemporaryDir storageTmp;
        QTemporaryDir knowledgeTmp;
        QVERIFY(storageTmp.isValid());
        QVERIFY(knowledgeTmp.isValid());

        const QString wav = storageTmp.filePath(QStringLiteral("export_test.wav"));
        {
            QFile f(wav);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(VoiceMemosController::createSyntheticWav(3, 16000));
            f.close();
        }

        VoiceMemosController ctrl(storageTmp.path(), knowledgeTmp.path());
        const QString id = ctrl.addMemoFromWav(
            wav,
            QStringLiteral("Reunión Sprint Semanal"),
            QStringLiteral("Revisamos los objetivos de KDE Plasma 6.8 y Wayland."),
            QStringLiteral("- Objetivos Plasma 6.8\n- Soporte Wayland"));

        QVERIFY(ctrl.exportToKiriKnowledge(id));

        QDir kDir(knowledgeTmp.path());
        const QStringList mdFiles = kDir.entryList({QStringLiteral("*.md")}, QDir::Files);
        QCOMPARE(mdFiles.size(), 1);

        QFile mdFile(kDir.filePath(mdFiles.first()));
        QVERIFY(mdFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(mdFile.readAll());
        QVERIFY(content.contains(QStringLiteral("# Reunión Sprint Semanal")));
        QVERIFY(content.contains(QStringLiteral("Objetivos Plasma 6.8")));
        QVERIFY(content.contains(QStringLiteral("Revisamos los objetivos de KDE Plasma 6.8")));
    }

    void testQmlLoadsWithoutErrors()
    {
        QTemporaryDir storageTmp;
        QTemporaryDir knowledgeTmp;
        VoiceMemosController ctrl(storageTmp.path(), knowledgeTmp.path());

        QQmlEngine engine;
        engine.addImportPath(QStringLiteral("/usr/lib/qt6/qml"));
        engine.rootContext()->setContextProperty(QStringLiteral("VoiceMemos"), &ctrl);

        QQmlComponent comp(&engine, QUrl(QStringLiteral("qrc:/org/cesar/kvoicememos/qml/main.qml")));
        QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
    }

    void testGoldenGateFeatures()
    {
        QTemporaryDir storageTmp;
        QTemporaryDir knowledgeTmp;
        QVERIFY(storageTmp.isValid());
        QVERIFY(knowledgeTmp.isValid());

        const QString wav = storageTmp.filePath(QStringLiteral("sample_8s.wav"));
        {
            QFile f(wav);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(VoiceMemosController::createSyntheticWav(8, 16000));
            f.close();
        }

        // Waveform peak extraction
        const QVariantList peaks = VoiceMemosController::extractWaveformPeaks(wav, 64);
        QCOMPARE(peaks.size(), 64);
        for (const QVariant &p : peaks) {
            const double val = p.toDouble();
            QVERIFY(val >= 0.05 && val <= 1.0);
        }

        VoiceMemosController ctrl(storageTmp.path(), knowledgeTmp.path());
        const QString id = ctrl.addMemoFromWav(
            wav,
            QStringLiteral("Ensayo Golden Gate"),
            QStringLiteral("Transcripción de prueba"),
            QStringLiteral("- Punto clave"));

        // Folders & filtering
        QVERIFY(ctrl.createFolder(QStringLiteral("Reuniones")));
        QVERIFY(!ctrl.createFolder(QStringLiteral("all")));
        QCOMPARE(ctrl.folders().size(), 1);
        ctrl.moveMemoToFolder(id, QStringLiteral("Reuniones"));
        ctrl.setSelectedFolder(QStringLiteral("Reuniones"));
        QCOMPARE(ctrl.memos().size(), 1);
        QVERIFY(ctrl.renameFolder(QStringLiteral("Reuniones"), QStringLiteral("Trabajo")));
        QCOMPARE(ctrl.selectedFolder(), QStringLiteral("Trabajo"));
        QVERIFY(ctrl.deleteFolder(QStringLiteral("Trabajo")));
        QCOMPARE(ctrl.selectedFolder(), QStringLiteral("all"));

        // Favorites & duplicate & sorting
        ctrl.toggleFavorite(id);
        QCOMPARE(ctrl.favoritesCount(), 1);
        const QString copyId = ctrl.duplicateMemo(id);
        QVERIFY(!copyId.isEmpty());
        QVERIFY(copyId != id);
        QCOMPARE(ctrl.allCount(), 2);
        QVERIFY(QFile::exists(ctrl.selectedMemo().value(QStringLiteral("filePath")).toString()));

        ctrl.setSortBy(QStringLiteral("title"));
        QCOMPARE(ctrl.sortBy(), QStringLiteral("title"));
        ctrl.setSortBy(QStringLiteral("duration"));
        QCOMPARE(ctrl.sortBy(), QStringLiteral("duration"));

        // Trim WAV (save as new & in-place)
        QVERIFY(ctrl.trimMemo(id, 1.0, 4.0, true));
        QCOMPARE(ctrl.allCount(), 3);
        QCOMPARE(ctrl.selectedMemo().value(QStringLiteral("durationSec")).toInt(), 3);

        QVERIFY(ctrl.trimMemo(id, 0.5, 3.5, false));
        ctrl.selectMemo(id);
        QCOMPARE(ctrl.selectedMemo().value(QStringLiteral("durationSec")).toInt(), 3);

        // Trash -> Restore -> Empty Trash
        ctrl.moveToTrash(copyId);
        QCOMPARE(ctrl.trashCount(), 1);
        ctrl.restoreMemo(copyId);
        QCOMPARE(ctrl.trashCount(), 0);
        ctrl.moveToTrash(copyId);
        QCOMPARE(ctrl.trashCount(), 1);
        ctrl.emptyTrash();
        QCOMPARE(ctrl.trashCount(), 0);

        // Playback & recording properties
        ctrl.setPlaybackSpeed(1.5);
        QCOMPARE(ctrl.playbackSpeed(), 1.5);
        ctrl.setSkipSilence(true);
        QVERIFY(ctrl.skipSilence());
        ctrl.setEnhanceRecording(true);
        QVERIFY(ctrl.enhanceRecording());
        ctrl.setAutoTranscribeOnStop(false);
        QVERIFY(!ctrl.autoTranscribeOnStop());
        QCOMPARE(ctrl.formatElapsedPrecise(6740), QStringLiteral("00:06.74"));
    }

    void testToolbarButtonsAndPopoversInteractive()
    {
        QTemporaryDir storageTmp;
        QTemporaryDir knowledgeTmp;
        QVERIFY(storageTmp.isValid());
        QVERIFY(knowledgeTmp.isValid());

        const QString wav = storageTmp.filePath(QStringLiteral("interactive.wav"));
        {
            QFile f(wav);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(VoiceMemosController::createSyntheticWav(5, 16000));
            f.close();
        }

        VoiceMemosController ctrl(storageTmp.path(), knowledgeTmp.path());
        ctrl.addMemoFromWav(wav, QStringLiteral("Prueba Toolbar"), QStringLiteral("Hola mundo"), QStringLiteral("- Hola"));

        QQmlEngine engine;
        engine.addImportPath(QStringLiteral("/usr/lib/qt6/qml"));
        engine.rootContext()->setContextProperty(QStringLiteral("VoiceMemos"), &ctrl);

        QQmlComponent comp(&engine, QUrl(QStringLiteral("qrc:/org/cesar/kvoicememos/qml/main.qml")));
        QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));

        QScopedPointer<QObject> obj(comp.create());
        auto *win = qobject_cast<QQuickWindow *>(obj.data());
        QVERIFY(win != nullptr);
        QVERIFY(QTest::qWaitForWindowExposed(win));
        QCoreApplication::processEvents();

        auto clickButton = [win](QQuickItem *btn) {
            QTest::qWait(20);
            const QPoint pos = btn->mapToScene(QPointF(btn->width() / 2.0, btn->height() / 2.0)).toPoint();
            QTest::mouseClick(win, Qt::LeftButton, Qt::NoModifier, pos);
            QTest::qWait(20);
        };

        auto assertNoTruncatedText = [](QQuickItem *rootItem, const char *label) {
            QVERIFY(rootItem != nullptr);
            int visibleTexts = 0;
            std::function<void(QQuickItem *)> visit = [&](QQuickItem *item) {
                if (!item || !item->isVisible()) {
                    return;
                }
                if (item->inherits("QQuickText")) {
                    const QVariant truncatedVar = item->property("truncated");
                    if (truncatedVar.isValid()) {
                        ++visibleTexts;
                        QVERIFY2(!truncatedVar.toBool(),
                                 qPrintable(QStringLiteral("%1 truncated text: '%2' (w=%3, impW=%4)")
                                                .arg(QLatin1String(label))
                                                .arg(item->property("text").toString())
                                                .arg(item->width())
                                                .arg(item->implicitWidth())));
                    }
                }
                for (QQuickItem *child : item->childItems()) {
                    visit(child);
                }
            };
            visit(rootItem);
            QVERIFY2(visibleTexts > 0, label);
        };

        auto *playbackOptsBtn = win->findChild<QQuickItem *>(QStringLiteral("playbackOptsBtn"));
        auto *transcriptBtn = win->findChild<QQuickItem *>(QStringLiteral("transcriptBtn"));
        auto *trimBtn = win->findChild<QQuickItem *>(QStringLiteral("trimBtn"));
        auto *summaryBtn = win->findChild<QQuickItem *>(QStringLiteral("summaryBtn"));
        auto *shareBtn = win->findChild<QQuickItem *>(QStringLiteral("shareBtn"));
        auto *moreBtn = win->findChild<QQuickItem *>(QStringLiteral("moreBtn"));
        auto *playbackPopover = win->findChild<QObject *>(QStringLiteral("playbackPopover"));
        auto *sharePopover = win->findChild<QObject *>(QStringLiteral("sharePopover"));
        auto *memoMoreMenu = win->findChild<QObject *>(QStringLiteral("memoMoreMenu"));

        QVERIFY(playbackOptsBtn && transcriptBtn && trimBtn && summaryBtn && shareBtn && moreBtn);
        QVERIFY(playbackPopover && sharePopover && memoMoreMenu);
        QVERIFY(playbackOptsBtn->isEnabled() && transcriptBtn->isEnabled() && trimBtn->isEnabled());
        QVERIFY(summaryBtn->isEnabled() && shareBtn->isEnabled() && moreBtn->isEnabled());

        // 1. Sliders -> playbackPopover
        clickButton(playbackOptsBtn);
        QCOMPARE(playbackPopover->property("opened").toBool(), true);
        QVERIFY(playbackPopover->property("height").toReal() >= 120.0);
        const qreal optsBottom = playbackOptsBtn->mapToScene(QPointF(0, playbackOptsBtn->height())).y();
        QVERIFY(playbackPopover->property("y").toReal() >= optsBottom);
        assertNoTruncatedText(qvariant_cast<QQuickItem *>(playbackPopover->property("contentItem")), "playbackPopover");
        QVERIFY(QMetaObject::invokeMethod(playbackPopover, "close"));
        QCoreApplication::processEvents();

        // 2. Speech bubble -> Transcript panel
        clickButton(transcriptBtn);
        QCOMPARE(win->property("activePanel").toString(), QStringLiteral("transcript"));
        clickButton(transcriptBtn);
        QCOMPARE(win->property("activePanel").toString(), QStringLiteral("waveform"));

        // 3. Crop -> Trim mode
        clickButton(trimBtn);
        QCOMPARE(win->property("trimMode").toBool(), true);
        clickButton(trimBtn);
        QCOMPARE(win->property("trimMode").toBool(), false);

        // 4. Sparkles -> AI Summary panel
        clickButton(summaryBtn);
        QCOMPARE(win->property("activePanel").toString(), QStringLiteral("summary"));
        clickButton(summaryBtn);
        QCOMPARE(win->property("activePanel").toString(), QStringLiteral("waveform"));

        // 5. Share -> sharePopover
        clickButton(shareBtn);
        QCOMPARE(sharePopover->property("opened").toBool(), true);
        QVERIFY(sharePopover->property("height").toReal() >= 120.0);
        assertNoTruncatedText(qvariant_cast<QQuickItem *>(sharePopover->property("contentItem")), "sharePopover");
        QVERIFY(QMetaObject::invokeMethod(sharePopover, "close"));
        QCoreApplication::processEvents();

        // 6. More (...) -> memoMoreMenu
        clickButton(moreBtn);
        QCOMPARE(memoMoreMenu->property("opened").toBool(), true);
        QVERIFY(memoMoreMenu->property("width").toReal() > 200.0);
        const QQuickItem *menuParent = memoMoreMenu->property("parent").value<QQuickItem *>();
        QVERIFY(menuParent != nullptr);
        const QPointF menuScenePos = menuParent->mapToScene(QPointF(memoMoreMenu->property("x").toReal(), memoMoreMenu->property("y").toReal()));
        QVERIFY(menuScenePos.x() >= 6.0);
        QVERIFY(menuScenePos.x() + memoMoreMenu->property("width").toReal() <= win->width() - 6.0);
        assertNoTruncatedText(qvariant_cast<QQuickItem *>(memoMoreMenu->property("contentItem")), "memoMoreMenu");
        QVERIFY(QMetaObject::invokeMethod(memoMoreMenu, "close"));
        QCoreApplication::processEvents();
    }
};

QTEST_MAIN(TestKVoiceMemos)
#include "test_kvoicememos.moc"
