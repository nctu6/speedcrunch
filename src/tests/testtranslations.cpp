// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QTest>
#include <QTranslator>

class TestTranslations : public QObject {
    Q_OBJECT

private slots:
    void catalogs_match_translation_sources();
    void localized_text_data();
    void localized_text();
    void english_uses_source_text();
};

void TestTranslations::catalogs_match_translation_sources()
{
    QStringList expected = QDir(QStringLiteral(SPEEDCRUNCH_TS_DIRECTORY))
                               .entryList({QStringLiteral("*.ts")}, QDir::Files, QDir::Name);
    QVERIFY(!expected.isEmpty());
    for (QString& name : expected)
        name.replace(name.size() - 3, 3, QStringLiteral(".qm"));

    const QDir resources(QStringLiteral(":/locale"));
    QCOMPARE(resources.entryList({QStringLiteral("*.qm")}, QDir::Files, QDir::Name), expected);
    for (const QString& name : expected) {
        // The English template can be empty and use source-text fallback.
        if (name == QStringLiteral("en_US.qm"))
            continue;
        QTranslator translator;
        QVERIFY2(translator.load(resources.filePath(name)), qPrintable(name));
        QVERIFY2(!translator.isEmpty(), qPrintable(name));
    }
}

void TestTranslations::localized_text_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("expected");
    QTest::newRow("generic German") << QStringLiteral("de_DE") << QStringLiteral("Sprache");
    QTest::newRow("Portugal") << QStringLiteral("pt_PT") << QStringLiteral("Idioma");
    QTest::newRow("Brazil") << QStringLiteral("pt_BR") << QStringLiteral("Idioma");
}

void TestTranslations::localized_text()
{
    QFETCH(QString, locale);
    QFETCH(QString, expected);
    QTranslator translator;
    QVERIFY(translator.load(QLocale(locale), QStringLiteral(":/locale/")));
    QVERIFY(QCoreApplication::installTranslator(&translator));
    const QString actual = QCoreApplication::translate("MainWindow", "Language");
    QCoreApplication::removeTranslator(&translator);
    QCOMPARE(actual, expected);
}

void TestTranslations::english_uses_source_text()
{
    QVERIFY(QFile::exists(QStringLiteral(":/locale/en_US.qm")));
    QTranslator translator;
    if (translator.load(QLocale(QStringLiteral("en_US")), QStringLiteral(":/locale/")))
        QVERIFY(QCoreApplication::installTranslator(&translator));
    const QString actual = QCoreApplication::translate("MainWindow", "Language");
    QCoreApplication::removeTranslator(&translator);
    QCOMPARE(actual, QStringLiteral("Language"));
}

QTEST_GUILESS_MAIN(TestTranslations)
#include "testtranslations.moc"
