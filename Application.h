/*
 Copyright (c) 2012-2015 Nion Company.
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QtWidgets/QApplication>
#include <QtCore/QVariant>

#ifndef USE_THRIFT
#define USE_THRIFT 0
#endif

#if USE_THRIFT
#include "GUI_types.h"
#include "GUICallbacks.h"
#endif

class DocumentWindow;

typedef QList<DocumentWindow *> DocumentWindowList;

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int & argv, char **args);

    static Application *instance() { return static_cast<Application *>(QCoreApplication::instance()); }

    bool initialize();

    // useful for locating resources
    QString resourcesPath() const;

    // Python related methods
    QVariant lookupPyObjectByName(const QString &object);
    QVariant invokePyMethod(const QVariant &object, const QString &method, const QVariantList &args);
    QVariant dispatchPyMethod(const QVariant &object, const QString &method, const QVariantList &args);
    bool setPyObjectAttribute(const QVariant &object, const QString &attribute, const QVariant &value);
    QVariant getPyObjectAttribute(const QVariant &object, const QString &attribute);

#if USE_THRIFT
    GUICallbacksClient *callbacks;
#endif

public Q_SLOTS:
    void output(const QString &str);

private Q_SLOTS:
    void continueQuit();
    void aboutToQuit();

private:
    QString m_python_home;
    QString m_python_target;

    QVariant m_bootstrap_module;

    QVariant m_py_application;

    bool m_quit_on_last_window;

    friend class DocumentWindow;
};

#endif // APPLICATION_H
