/* This file is part of the KDE project
   Copyright (C) 2013 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "globaltest.h"
#include <QTest>

#include "global.h"
#include "kioglobal_p.h"

#include <QFile>
#include <QTemporaryDir>

#include <sys/stat.h>

QTEST_MAIN(GlobalTest)

void GlobalTest::testUserPermissionConversion()
{
    const int permissions = S_IRUSR | S_IWUSR | S_IXUSR;
    QFile::Permissions qPermissions = KIO::convertPermissions(permissions);

    QFile::Permissions perms = (QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QCOMPARE(qPermissions & perms, perms);

    perms = (QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup);
    QCOMPARE(qPermissions & perms, 0);

    perms = (QFile::ReadOther | QFile::WriteOther | QFile::ExeOther);
    QCOMPARE(qPermissions & perms, 0);
}

void GlobalTest::testGroupPermissionConversion()
{
    const int permissions = S_IRGRP | S_IWGRP | S_IXGRP;
    QFile::Permissions qPermissions = KIO::convertPermissions(permissions);

    QFile::Permissions perms = (QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QCOMPARE(qPermissions & perms, 0);

    perms = (QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup);
    QCOMPARE(qPermissions & perms, perms);

    perms = (QFile::ReadOther | QFile::WriteOther | QFile::ExeOther);
    QCOMPARE(qPermissions & perms, 0);
}

void GlobalTest::testOtherPermissionConversion()
{
    const int permissions = S_IROTH | S_IWOTH | S_IXOTH;
    QFile::Permissions qPermissions = KIO::convertPermissions(permissions);

    QFile::Permissions perms = (QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QCOMPARE(qPermissions & perms, 0);

    perms = (QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup);
    QCOMPARE(qPermissions & perms, 0);

    perms = (QFile::ReadOther | QFile::WriteOther | QFile::ExeOther);
    QCOMPARE(qPermissions & perms, perms);
}

void GlobalTest::testSuggestName_data()
{
    QTest::addColumn<QString>("oldName");
    QTest::addColumn<QStringList>("existingFiles");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("non-existing") << "foobar" << QStringList() << "foobar 1";
    QTest::newRow("existing") << "foobar" << QStringList("foobar") << "foobar 1";
    QTest::newRow("existing_1") << "foobar" << (QStringList() << "foobar" << "foobar 1") << "foobar 2";
    QTest::newRow("extension") << "foobar.txt" << QStringList() << "foobar 1.txt";
    QTest::newRow("extension_exists") << "foobar.txt" << (QStringList() << "foobar.txt") << "foobar 1.txt";
    QTest::newRow("extension_exists_1") << "foobar.txt" << (QStringList() << "foobar.txt" << "foobar 1.txt") << "foobar 2.txt";
    QTest::newRow("two_extensions") << "foobar.tar.gz" << QStringList() << "foobar 1.tar.gz";
    QTest::newRow("two_extensions_exists") << "foobar.tar.gz" << (QStringList() << "foobar.tar.gz") << "foobar 1.tar.gz";
    QTest::newRow("two_extensions_exists_1") << "foobar.tar.gz" << (QStringList() << "foobar.tar.gz" << "foobar 1.tar.gz") << "foobar 2.tar.gz";
    QTest::newRow("with_space") << "foo bar" << QStringList("foo bar") << "foo bar 1";
    QTest::newRow("dot_at_beginning") << ".aFile.tar.gz" << QStringList() << ".aFile 1.tar.gz";
    QTest::newRow("dots_at_beginning") << "..aFile.tar.gz" << QStringList() << "..aFile 1.tar.gz";
    QTest::newRow("empty_basename") << ".txt" << QStringList() << ".txt 1";
    QTest::newRow("hidden_file") << ".foo" << QStringList() << ".foo 1";
    QTest::newRow("empty_basename_2dots") << "..txt" << QStringList() << "..txt 1";
    QTest::newRow("hidden_file_2dots") << "..foo" << QStringList() << "..foo 1";
}

void GlobalTest::testSuggestName()
{
    QFETCH(QString, oldName);
    QFETCH(QStringList, existingFiles);
    QFETCH(QString, expectedOutput);

    QTemporaryDir dir;
    const QUrl baseUrl = QUrl::fromLocalFile(dir.path());
    foreach (const QString &localFile, existingFiles) {
        QFile file(dir.path() + '/' + localFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
    }
    QCOMPARE(KIO::suggestName(baseUrl, oldName), expectedOutput);
}

#include "globaltest.moc"
