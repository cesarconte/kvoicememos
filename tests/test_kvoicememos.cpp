/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "voicememoscontroller.h"

#include <QDir>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
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
        engine.rootContext()->setContextProperty(QStringLiteral("VoiceMemos"), &ctrl);

        QQmlComponent comp(&engine, QUrl(QStringLiteral("qrc:/org/cesar/kvoicememos/qml/main.qml")));
        QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
    }
};

QTEST_MAIN(TestKVoiceMemos)
#include "test_kvoicememos.moc"
