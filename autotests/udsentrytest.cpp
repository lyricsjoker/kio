/* This file is part of the KDE project
   Copyright (C) 2013 Frank Reininghaus <frank78ac@googlemail.com>

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

#include "udsentrytest.h"

#include <QTest>
#include <QVector>
#include <QDataStream>
#include <QTemporaryFile>
#include <qplatformdefs.h>

#include <kfileitem.h>
#include <udsentry.h>

#include "kiotesthelper.h"

struct UDSTestField {
    UDSTestField() {}

    UDSTestField(uint uds, const QString &value) :
        m_uds(uds),
        m_string(value)
    {
        Q_ASSERT(uds & KIO::UDSEntry::UDS_STRING);
    }

    UDSTestField(uint uds, long long value) :
        m_uds(uds),
        m_long(value)
    {
        Q_ASSERT(uds & KIO::UDSEntry::UDS_NUMBER);
    }

    uint m_uds;
    QString m_string;
    long long m_long;
};

/**
 * Test that storing UDSEntries to a stream and then re-loading them works.
 */
void UDSEntryTest::testSaveLoad()
{
    QVector<QVector<UDSTestField> > testCases;
    QVector<UDSTestField> testCase;

    // Data for 1st UDSEntry.
    testCase
            << UDSTestField(KIO::UDSEntry::UDS_SIZE, 1)
            << UDSTestField(KIO::UDSEntry::UDS_USER, QStringLiteral("user1"))
            << UDSTestField(KIO::UDSEntry::UDS_GROUP, QStringLiteral("group1"))
            << UDSTestField(KIO::UDSEntry::UDS_NAME, QStringLiteral("filename1"))
            << UDSTestField(KIO::UDSEntry::UDS_MODIFICATION_TIME, 123456)
            << UDSTestField(KIO::UDSEntry::UDS_CREATION_TIME, 12345)
            << UDSTestField(KIO::UDSEntry::UDS_DEVICE_ID, 2)
            << UDSTestField(KIO::UDSEntry::UDS_INODE, 56);
    testCases << testCase;

    // 2nd entry: change some of the data.
    testCase.clear();
    testCase
            << UDSTestField(KIO::UDSEntry::UDS_SIZE, 2)
            << UDSTestField(KIO::UDSEntry::UDS_USER, QStringLiteral("user2"))
            << UDSTestField(KIO::UDSEntry::UDS_GROUP, QStringLiteral("group1"))
            << UDSTestField(KIO::UDSEntry::UDS_NAME, QStringLiteral("filename2"))
            << UDSTestField(KIO::UDSEntry::UDS_MODIFICATION_TIME, 12345)
            << UDSTestField(KIO::UDSEntry::UDS_CREATION_TIME, 1234)
            << UDSTestField(KIO::UDSEntry::UDS_DEVICE_ID, 87)
            << UDSTestField(KIO::UDSEntry::UDS_INODE, 42);
    testCases << testCase;

    // 3rd entry: keep the data, but change the order of the entries.
    testCase.clear();
    testCase
            << UDSTestField(KIO::UDSEntry::UDS_SIZE, 2)
            << UDSTestField(KIO::UDSEntry::UDS_GROUP, QStringLiteral("group1"))
            << UDSTestField(KIO::UDSEntry::UDS_USER, QStringLiteral("user2"))
            << UDSTestField(KIO::UDSEntry::UDS_NAME, QStringLiteral("filename2"))
            << UDSTestField(KIO::UDSEntry::UDS_MODIFICATION_TIME, 12345)
            << UDSTestField(KIO::UDSEntry::UDS_DEVICE_ID, 87)
            << UDSTestField(KIO::UDSEntry::UDS_INODE, 42)
            << UDSTestField(KIO::UDSEntry::UDS_CREATION_TIME, 1234);
    testCases << testCase;

    // 4th entry: change some of the data and the order of the entries.
    testCase.clear();
    testCase
            << UDSTestField(KIO::UDSEntry::UDS_SIZE, 2)
            << UDSTestField(KIO::UDSEntry::UDS_USER, QStringLiteral("user4"))
            << UDSTestField(KIO::UDSEntry::UDS_GROUP, QStringLiteral("group4"))
            << UDSTestField(KIO::UDSEntry::UDS_MODIFICATION_TIME, 12346)
            << UDSTestField(KIO::UDSEntry::UDS_DEVICE_ID, 87)
            << UDSTestField(KIO::UDSEntry::UDS_INODE, 42)
            << UDSTestField(KIO::UDSEntry::UDS_CREATION_TIME, 1235)
            << UDSTestField(KIO::UDSEntry::UDS_NAME, QStringLiteral("filename4"));
    testCases << testCase;

    // 5th entry: remove one field.
    testCase.clear();
    testCase
            << UDSTestField(KIO::UDSEntry::UDS_SIZE, 2)
            << UDSTestField(KIO::UDSEntry::UDS_USER, QStringLiteral("user4"))
            << UDSTestField(KIO::UDSEntry::UDS_GROUP, QStringLiteral("group4"))
            << UDSTestField(KIO::UDSEntry::UDS_MODIFICATION_TIME, 12346)
            << UDSTestField(KIO::UDSEntry::UDS_INODE, 42)
            << UDSTestField(KIO::UDSEntry::UDS_CREATION_TIME, 1235)
            << UDSTestField(KIO::UDSEntry::UDS_NAME, QStringLiteral("filename4"));
    testCases << testCase;

    // 6th entry: add a new field, and change some others.
    testCase.clear();
    testCase
            << UDSTestField(KIO::UDSEntry::UDS_SIZE, 89)
            << UDSTestField(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("icon6"))
            << UDSTestField(KIO::UDSEntry::UDS_USER, QStringLiteral("user6"))
            << UDSTestField(KIO::UDSEntry::UDS_GROUP, QStringLiteral("group4"))
            << UDSTestField(KIO::UDSEntry::UDS_MODIFICATION_TIME, 12346)
            << UDSTestField(KIO::UDSEntry::UDS_INODE, 32)
            << UDSTestField(KIO::UDSEntry::UDS_CREATION_TIME, 1235)
            << UDSTestField(KIO::UDSEntry::UDS_NAME, QStringLiteral("filename6"));
    testCases << testCase;

    // Store the entries in a QByteArray.
    QByteArray data;
    {
        QDataStream stream(&data, QIODevice::WriteOnly);
        foreach (const QVector<UDSTestField> &testCase, testCases) {
            KIO::UDSEntry entry;

            foreach (const UDSTestField &field, testCase) {
                uint uds = field.m_uds;
                if (uds & KIO::UDSEntry::UDS_STRING) {
                    entry.insert(uds, field.m_string);
                } else {
                    Q_ASSERT(uds & KIO::UDSEntry::UDS_NUMBER);
                    entry.insert(uds, field.m_long);
                }
            }

            QCOMPARE(entry.count(), testCase.count());
            stream << entry;
        }
    }

    // Re-load the entries and compare with the data in testCases.
    {
        QDataStream stream(data);
        foreach (const QVector<UDSTestField> &testCase, testCases) {
            KIO::UDSEntry entry;
            stream >> entry;
            QCOMPARE(entry.count(), testCase.count());

            foreach (const UDSTestField &field, testCase) {
                uint uds = field.m_uds;
                QVERIFY(entry.contains(uds));

                if (uds & KIO::UDSEntry::UDS_STRING) {
                    QCOMPARE(entry.stringValue(uds), field.m_string);
                } else {
                    Q_ASSERT(uds & KIO::UDSEntry::UDS_NUMBER);
                    QCOMPARE(entry.numberValue(uds), field.m_long);
                }
            }
        }
    }

    // Now: Store the fields manually in the order in which they appear in
    // testCases, and re-load them. This ensures that loading the entries
    // works no matter in which order the fields appear in the QByteArray.
    data.clear();

    {
        QDataStream stream(&data, QIODevice::WriteOnly);
        foreach (const QVector<UDSTestField> &testCase, testCases) {
            stream << testCase.count();

            foreach (const UDSTestField &field, testCase) {
                uint uds = field.m_uds;
                stream << uds;

                if (uds & KIO::UDSEntry::UDS_STRING) {
                    stream << field.m_string;
                } else {
                    Q_ASSERT(uds & KIO::UDSEntry::UDS_NUMBER);
                    stream << field.m_long;
                }
            }
        }
    }

    {
        QDataStream stream(data);
        foreach (const QVector<UDSTestField> &testCase, testCases) {
            KIO::UDSEntry entry;
            stream >> entry;
            QCOMPARE(entry.count(), testCase.count());

            foreach (const UDSTestField &field, testCase) {
                uint uds = field.m_uds;
                QVERIFY(entry.contains(uds));

                if (uds & KIO::UDSEntry::UDS_STRING) {
                    QCOMPARE(entry.stringValue(uds), field.m_string);
                } else {
                    Q_ASSERT(uds & KIO::UDSEntry::UDS_NUMBER);
                    QCOMPARE(entry.numberValue(uds), field.m_long);
                }
            }
        }
    }
}

/**
 * Test to verify that move semantics work. This is only useful when ran through callgrind.
 */
void UDSEntryTest::testMove()
{
    // Create a temporary file. Just to make a UDSEntry further down.
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray filePath = file.fileName().toLocal8Bit();
    const QString fileName = QUrl(file.fileName()).fileName(); // QTemporaryFile::fileName returns the full path.
    QVERIFY(!fileName.isEmpty());

    // We have a file now. Get the stat data from it to make the UDSEntry.
    QT_STATBUF statBuf;
    QVERIFY(QT_LSTAT(filePath.constData(), &statBuf) == 0);
    KIO::UDSEntry entry(statBuf, fileName);

    // Verify that the name in the UDSEntry is the same as we've got from the fileName var.
    QCOMPARE(fileName, entry.stringValue(KIO::UDSEntry::UDS_NAME));

    // That was the boilerplate code. Now for move semantics.
    // First: move assignment.
    {
        // First a copy as we need to keep the entry for the next test.
        KIO::UDSEntry entryCopy = entry;

        // Now move-assignement (two lines to prevent compiler optimization)
        KIO::UDSEntry movedEntry;
        movedEntry = std::move(entryCopy);

        // And veryfy that this works.
        QCOMPARE(fileName, movedEntry.stringValue(KIO::UDSEntry::UDS_NAME));
    }

    // Move constructor
    {
        // First a copy again
        KIO::UDSEntry entryCopy = entry;

        // Now move-assignement
        KIO::UDSEntry movedEntry(std::move(entryCopy));

        // And veryfy that this works.
        QCOMPARE(fileName, movedEntry.stringValue(KIO::UDSEntry::UDS_NAME));
    }
}

QTEST_MAIN(UDSEntryTest)
