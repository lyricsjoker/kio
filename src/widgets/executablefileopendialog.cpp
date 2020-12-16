/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2014 Arjun A.K. <arjunak234@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "executablefileopendialog_p.h"

#include <QCheckBox>
#include <QLabel>
#include <QMimeType>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include <KLocalizedString>

ExecutableFileOpenDialog::ExecutableFileOpenDialog(ExecutableFileOpenDialog::Mode mode,
                                                   const QMimeType &mimeType, QWidget *parent)
  : QDialog(parent)
{
    QString text;
    if (mimeType.inherits(QLatin1String("application/x-desktop"))) {
        text = i18n("What do you wish to do with this desktop file?");
    } else {
        text = i18n("What do you wish to do with this executable file?");
    }

    QLabel *label = new QLabel(text, this);

    m_dontAskAgain = new QCheckBox(this);
    m_dontAskAgain->setText(i18n("Do not ask again"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);

    QPushButton *openButton;
    if (mode == OpenOrExecute) {
        openButton = new QPushButton(i18n("&Open"), this);
        openButton->setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));
        buttonBox->addButton(openButton, QDialogButtonBox::AcceptRole);
    }

    QPushButton *executeButton = new QPushButton(i18n("&Execute"), this);
    executeButton->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
    buttonBox->addButton(executeButton, QDialogButtonBox::AcceptRole);

    buttonBox->button(QDialogButtonBox::Cancel)->setFocus();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(m_dontAskAgain);
    layout->addWidget(buttonBox);

    if (mode == OnlyExecute) {
        connect(executeButton, &QPushButton::clicked, [=]{done(ExecuteFile);});
    } else if (mode == OpenAsExecute) {
        connect(executeButton, &QPushButton::clicked, [=]{done(OpenFile);});
    } else {
        connect(openButton, &QPushButton::clicked, [=]{done(OpenFile);});
        connect(executeButton, &QPushButton::clicked, [=]{done(ExecuteFile);});
    }
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ExecutableFileOpenDialog::reject);
}

ExecutableFileOpenDialog::ExecutableFileOpenDialog(QWidget *parent) :
    ExecutableFileOpenDialog(ExecutableFileOpenDialog::OpenOrExecute, QMimeType{}, parent) { }

bool ExecutableFileOpenDialog::isDontAskAgainChecked() const
{
    return m_dontAskAgain->isChecked();
}

