#ifndef PYTHONSELECTDIALOG_H
#define PYTHONSELECTDIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

class PythonSelectDialog : public QDialog
{
    Q_OBJECT

public:
    PythonSelectDialog();
    QString getPythonHome() const { return m_python_home; }

public Q_SLOTS:
    void selectDirectory();

private:
    QString m_python_home;
    QLabel *directoryLabel;
};

#endif
