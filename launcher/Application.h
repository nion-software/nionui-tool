/*
 Copyright (c) 2012-2015 Nion Company.
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QSplashScreen>
#include <QtCore/QFile>
#include <QtCore/QVariant>

float GetDisplayScaling();

class DocumentWindow;

typedef QList<DocumentWindow *> DocumentWindowList;

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int & argv, char **args);

    static Application *instance() { return static_cast<Application *>(QCoreApplication::instance()); }

    bool initialize();
    void deinitialize();

    // useful for locating resources
    QString resourcesPath() const;

    // Python related methods
    bool hasPyMethod(const QVariant &object, const QString &method);
    QVariant invokePyMethod(const QVariant &object, const QString &method, const QVariantList &args);
    QVariant dispatchPyMethod(const QVariant &object, const QString &method, const QVariantList &args);
    bool setPyObjectAttribute(const QVariant &object, const QString &attribute, const QVariant &value);
    QVariant getPyObjectAttribute(const QVariant &object, const QString &attribute);
    void closeSplashScreen();
    QFile &getLogFile() { return logFile; }

public Q_SLOTS:
    void output(const QString &str);

private Q_SLOTS:
    void aboutToQuit();

private:
    QScopedPointer<QSplashScreen> m_splash_screen;

    QFile logFile;

    QString m_python_home;
    QList<QString> m_python_paths;
    QString m_python_library;
    QString m_python_app;

    QVariant m_bootstrap_module;

    QVariant m_py_application;

    friend class DocumentWindow;
};

#endif // APPLICATION_H
