/*
    SPDX-FileCopyrightText: 2026 Cesar
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "voicememoscontroller.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kvoicememos"));
    app.setApplicationDisplayName(QStringLiteral("Notas de Voz (KVoiceMemos)"));
    app.setOrganizationDomain(QStringLiteral("cesar.org"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.cesar.kvoicememos"), QIcon(QStringLiteral(":/org/cesar/kvoicememos/icons/org.cesar.kvoicememos.svg"))));

    VoiceMemosController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("VoiceMemos"), &controller);
    engine.load(QUrl(QStringLiteral("qrc:/org/cesar/kvoicememos/qml/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
