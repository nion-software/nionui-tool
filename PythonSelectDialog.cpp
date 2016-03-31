#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>

#include "PythonSelectDialog.h"

PythonSelectDialog::PythonSelectDialog()
{
    QLabel *selectLabel = new QLabel("Please select a Python 3.5 directory with\nNumPy 1.10 and SciPy installed.");

    directoryLabel = new QLabel();

    QPushButton *selectButton = new QPushButton("Select Directory...");

    QPushButton *quitButton = new QPushButton("Quit");

    QPushButton *continueButton = new QPushButton("Continue");

    connect(continueButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(selectButton, SIGNAL(clicked()), this, SLOT(selectDirectory()));

    QHBoxLayout *selectButtonLayout = new QHBoxLayout;
    selectButtonLayout->addStretch(1);
    selectButtonLayout->addWidget(selectButton);
    selectButtonLayout->addStretch(1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(quitButton);
    buttonsLayout->addWidget(continueButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(selectLabel);
    mainLayout->addLayout(selectButtonLayout);
    mainLayout->addWidget(directoryLabel);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle("Select Python Directory");

    directoryLabel->setFixedWidth(300);
    directoryLabel->setFixedHeight(64);
    directoryLabel->setWordWrap(true);
}

void PythonSelectDialog::selectDirectory()
{
    QString result = QFileDialog::getExistingDirectory(NULL, "Please select Python 3.5 directory.");
    if (!result.isEmpty())
    {
        m_python_home = QDir::toNativeSeparators(result);
        directoryLabel->setText(m_python_home);
    }
}
