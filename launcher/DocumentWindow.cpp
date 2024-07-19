/*
 Copyright (c) 2012-2015 Nion Company.
 */

#include <stdint.h>

#if defined(__APPLE__)
#include <mach/mach_time.h> /* mach_absolute_time */
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <chrono>
#endif
#if defined(__linux__)
#include <time.h>
#endif

#include <QtCore/QtGlobal>
#include <QtCore/QAbstractListModel>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMimeData>
#include <QtCore/QQueue>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QSettings>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include <QtGui/QAction>
#include <QtGui/QFontDatabase>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QScreen>
#include <QtGui/QStyleHints>
#include <QtGui/QWindow>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGestureEvent>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include "Application.h"
#include "DocumentWindow.h"
#include "PythonSupport.h"

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#define LOG_EXCEPTION(ctx) qDebug() << "EXCEPTION";

Q_DECLARE_METATYPE(std::string)

// from application
PyObject *QVariantToPyObject(const QVariant &value);

const auto DEFAULT_RENDER_HINTS = QPainter::Antialiasing | QPainter::TextAntialiasing;

QFont ParseFontString(const QString &font_string, float display_scaling = 1.0);

QColor ParseColorString(const QString &color_string)
{
    QColor color;
    QRegularExpression re1("^rgba\\((\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+\\.\\d+)\\)$");
    QRegularExpression re2("^rgb\\((\\d+),\\s*(\\d+),\\s*(\\d+)\\)$");
    QRegularExpressionMatch match1 = re1.match(color_string);
    QRegularExpressionMatch match2 = re2.match(color_string);
    if (match1.hasMatch())
        color = QColor(match1.captured(1).toInt(), match1.captured(2).toInt(), match1.captured(3).toInt(), match1.captured(4).toFloat() * 255);
    else if (match2.hasMatch())
        color = QColor(match2.captured(1).toInt(), match2.captured(2).toInt(), match2.captured(3).toInt());
    else
        color = QColor(color_string);
    return color;
}

DocumentWindow::DocumentWindow(const QString &title, QWidget *parent)
    : QMainWindow(parent)
    , m_closed(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    //setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    //setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    //setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Set the window title plus the 'window modified placeholder'
    if (!title.isEmpty())
        setWindowTitle(title);

    // Set sizing for widgets.
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    connect(application()->styleHints(), SIGNAL(colorSchemeChanged(Qt::ColorScheme)), this, SLOT(colorSchemeChanged(Qt::ColorScheme)));


    cleanDocument();
}

void DocumentWindow::initialize()
{
    // start the timer event
    m_periodic_timer = startTimer(20);

    // reset it here until it is really modified
    cleanDocument();
}

Application *DocumentWindow::application() const
{
    return dynamic_cast<Application *>(QCoreApplication::instance());
}

void DocumentWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_periodic_timer && isVisible())
        application()->dispatchPyMethod(m_py_object, "periodic", QVariantList());
}

void DocumentWindow::hideEvent(QHideEvent *hide_event)
{
    if (this->windowHandle())
    {
        disconnect(this->windowHandle(), SIGNAL(screenChanged(QScreen *)));
    }

    if (m_screen)
    {
        disconnect(m_screen, SIGNAL(logicalDotsPerInchChanged(qreal)));
        disconnect(m_screen, SIGNAL(physicalDotsPerInchChanged(qreal)));
        m_screen = nullptr;
    }

    QMainWindow::hideEvent(hide_event);
}

void DocumentWindow::showEvent(QShowEvent *show_event)
{
    QMainWindow::showEvent(show_event);

    // tell python we're closing.
    application()->dispatchPyMethod(m_py_object, "aboutToShow", QVariantList());

    setFocus();

    application()->closeSplashScreen();

    this->winId(); // force windowHandle() to return a valid QWindow
    if (this->windowHandle())
    {
        connect(this->windowHandle(), SIGNAL(screenChanged(QScreen *)), this, SLOT(screenChanged(QScreen *)));
        screenChanged(this->windowHandle()->screen());
    }
}

void DocumentWindow::logicalDotsPerInchChanged(qreal dpi)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "logicalDPIChanged", QVariantList() << dpi);
}

void DocumentWindow::physicalDotsPerInchChanged(qreal dpi)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "physicalDPIChanged", QVariantList() << dpi);
}

void DocumentWindow::screenChanged(QScreen *screen)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "screenChanged", QVariantList());

    m_screen = screen;

    if (m_screen)
    {
        connect(m_screen, SIGNAL(logicalDotsPerInchChanged(qreal)), this, SLOT(logicalDotsPerInchChanged(qreal)));
        connect(m_screen, SIGNAL(physicalDotsPerInchChanged(qreal)), this, SLOT(physicalDotsPerInchChanged(qreal)));
        logicalDotsPerInchChanged(m_screen->logicalDotsPerInch());
        physicalDotsPerInchChanged(m_screen->physicalDotsPerInch());
    }
}

void DocumentWindow::colorSchemeChanged(Qt::ColorScheme colorScheme)
{
    QString color_scheme;
    switch (colorScheme)
    {
        case Qt::ColorScheme::Light:
            color_scheme = "light";
            break;
        case Qt::ColorScheme::Dark:
            color_scheme = "dark";
            break;
        default:
            color_scheme = "unknown";
            break;
    }

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "colorSchemeChanged", QVariantList() << color_scheme);
}

void DocumentWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    float display_scaling = GetDisplayScaling();

    application()->dispatchPyMethod(m_py_object, "sizeChanged", QVariantList() << int(event->size().width() / display_scaling) << int(event->size().height() / display_scaling));
}

void DocumentWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);

    float display_scaling = GetDisplayScaling();

    application()->dispatchPyMethod(m_py_object, "positionChanged", QVariantList() << int(event->pos().x() / display_scaling) << int(event->pos().y() / display_scaling));
}

void DocumentWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    switch(event->type())
    {
        case QEvent::ActivationChange:
            application()->dispatchPyMethod(m_py_object, "activationChanged", QVariantList() << isActiveWindow());
            break;
        default:
            break;
    }
}

void DocumentWindow::closeEvent(QCloseEvent *close_event)
{
    // see closing issue when closing from dock widget on OS X:
    // https://bugreports.qt.io/browse/QTBUG-43344

    if (!m_closed) {

        QString geometry = QString(saveGeometry().toHex().data());
        QString state = QString(saveState().toHex().data());

        // tell python we're closing.
        application()->dispatchPyMethod(m_py_object, "aboutToClose", QVariantList() << geometry << state);

        m_closed = true;
    }

    close_event->accept();
    // window will be automatically hidden, according to Qt documentation
}

void DocumentWindow::cleanDocument()
{
    setWindowModified(false);
}

void DocumentWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        if (m_py_object.isValid())
        {
            Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
            if (app->dispatchPyMethod(m_py_object, "keyPressed", QVariantList() << event->text() << event->key() << (int)event->modifiers()).toBool())
            {
                event->accept();
                return;
            }
        }
    }

    QMainWindow::keyPressEvent(event);
}

void DocumentWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyRelease)
    {
        if (m_py_object.isValid())
        {
            Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
            if (app->dispatchPyMethod(m_py_object, "keyReleased", QVariantList() << event->text() << event->key() << (int)event->modifiers()).toBool())
            {
                event->accept();
                return;
            }
        }
    }

    QMainWindow::keyReleaseEvent(event);
}

DockWidget::DockWidget(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
    , m_screen(nullptr)
{
}

void DockWidget::closeEvent(QCloseEvent *event)
{
    QDockWidget::closeEvent(event);

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "willClose", QVariantList());
}

void DockWidget::hideEvent(QHideEvent *event)
{
    if (this->windowHandle())
    {
        disconnect(this->windowHandle(), SIGNAL(screenChanged(QScreen *)));
    }

    if (m_screen)
    {
        disconnect(m_screen, SIGNAL(logicalDotsPerInchChanged(qreal)));
        disconnect(m_screen, SIGNAL(physicalDotsPerInchChanged(qreal)));
        m_screen = nullptr;
    }

    QDockWidget::hideEvent(event);

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "willHide", QVariantList());
}

void DockWidget::logicalDotsPerInchChanged(qreal dpi)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "logicalDPIChanged", QVariantList() << dpi);
}

void DockWidget::physicalDotsPerInchChanged(qreal dpi)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "physicalDPIChanged", QVariantList() << dpi);
}

void DockWidget::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);

    float display_scaling = GetDisplayScaling();

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "sizeChanged", QVariantList() << int(event->size().width() / display_scaling) << int(event->size().height()) / display_scaling);
}

void DockWidget::screenChanged(QScreen *screen)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "screenChanged", QVariantList());

    m_screen = screen;

    if (m_screen)
    {
        connect(m_screen, SIGNAL(logicalDotsPerInchChanged(qreal)), this, SLOT(logicalDotsPerInchChanged(qreal)));
        connect(m_screen, SIGNAL(physicalDotsPerInchChanged(qreal)), this, SLOT(physicalDotsPerInchChanged(qreal)));
        logicalDotsPerInchChanged(m_screen->logicalDotsPerInch());
        physicalDotsPerInchChanged(m_screen->physicalDotsPerInch());
    }
}

void DockWidget::showEvent(QShowEvent *event)
{
    QDockWidget::showEvent(event);

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "willShow", QVariantList());

    this->winId(); // force windowHandle() to return a valid QWindow
    if (this->windowHandle())
    {
        connect(this->windowHandle(), SIGNAL(screenChanged(QScreen *)), this, SLOT(screenChanged(QScreen *)));
        screenChanged(this->windowHandle()->screen());
    }
}

void DockWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusIn", QVariantList());
    }

    QDockWidget::focusInEvent(event);
}

void DockWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusOut", QVariantList());
    }

    QDockWidget::focusOutEvent(event);
}

PyPushButton::PyPushButton()
{
    connect(this, SIGNAL(clicked()), this, SLOT(clicked()));
}

void PyPushButton::clicked()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "clicked", QVariantList());
    }
}

PyRadioButton::PyRadioButton()
{
    connect(this, SIGNAL(clicked()), this, SLOT(clicked()));
}

void PyRadioButton::clicked()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "clicked", QVariantList());
    }
}

PyButtonGroup::PyButtonGroup()
{
    connect(this, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
}

void PyButtonGroup::buttonClicked(QAbstractButton *button)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "clicked", QVariantList() << id(button));
    }
}

PyCheckBox::PyCheckBox()
{
    connect(this, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
}

void PyCheckBox::stateChanged(int state)
{
    if (m_py_object.isValid())
    {
        QStringList state_names;
        state_names << "unchecked" << "partial" << "checked";
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "stateChanged", QVariantList() << state_names[state]);
    }
}

PyComboBox::PyComboBox()
{
    connect(this, SIGNAL(currentTextChanged(QString)), this, SLOT(currentTextChanged(QString)));
}

void PyComboBox::currentTextChanged(const QString &currentText)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "currentTextChanged", QVariantList() << currentText);
    }
}

void PyComboBox::wheelEvent(QWheelEvent* event)
{
    if (this->isExpanded())
    {
        //If we are expanded, treat as normal
        QComboBox::wheelEvent(event);
    }
    else
    {
        //If we are not expanded, discard
        event->ignore();
    }
}

bool PyComboBox::isExpanded()
{
    auto view = this->view();
    if (view == nullptr) 
        return false; //It can't be expanded if it doesn't exist.
    return view->isVisible();
}

PySlider::PySlider()
{
    setOrientation(Qt::Horizontal);
    setTracking(true);
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
    connect(this, SIGNAL(sliderPressed()), this, SLOT(sliderPressed()));
    connect(this, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()));
    connect(this, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)));
}

void PySlider::valueChanged(int value)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "valueChanged", QVariantList() << value);
    }
}

void PySlider::sliderPressed()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "sliderPressed", QVariantList());
    }
}

void PySlider::sliderReleased()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "sliderReleased", QVariantList());
    }
}

void PySlider::sliderMoved(int value)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "sliderMoved", QVariantList() << value);
    }
}

PyLineEdit::PyLineEdit()
{
    connect(this, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
    connect(this, SIGNAL(textEdited(QString)), this, SLOT(textEdited(QString)));
}

void PyLineEdit::editingFinished()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "editingFinished", QVariantList() << text());
    }
}

void PyLineEdit::textEdited(const QString &text)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "textEdited", QVariantList() << text);
    }
}

void PyLineEdit::keyPressEvent(QKeyEvent *key_event)
{
    if (key_event->type() == QEvent::KeyPress)
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        if (key_event->key() == Qt::Key_Escape)
        {
            if (m_py_object.isValid())
            {
                if (app->dispatchPyMethod(m_py_object, "escapePressed", QVariantList()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
        else if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter)
        {
            if (m_py_object.isValid())
            {
                if (app->dispatchPyMethod(m_py_object, "returnPressed", QVariantList()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
        else
        {
            if (m_py_object.isValid())
            {
                Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
                if (app->dispatchPyMethod(m_py_object, "keyPressed", QVariantList() << key_event->text() << key_event->key() << (int)key_event->modifiers()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
    }

    QLineEdit::keyPressEvent(key_event);
}

PyTextBrowser::PyTextBrowser()
{
    // links are handled by Python and the anchorClicked function.
    setOpenLinks(false);
    setOpenExternalLinks(false);
    connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
}

QVariant PyTextBrowser::loadResource(int type, const QUrl &name)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

        if (type == QTextDocument::ImageResource)
        {
            auto result = app->dispatchPyMethod(m_py_object, "loadImageResource", QVariantList() << name);
            if (result.isValid())
            {
                PyObjectPtr image_object(QVariantToPyObject(result));

                QImageInterface image;
                PythonSupport::instance()->imageFromRGBA(image_object, &image);

                if (!image.image.isNull())
                {
                    return image.image;
                }
            }

        }
    }
    return QTextBrowser::QTextEdit::loadResource(type, name);
}

void PyTextBrowser::keyPressEvent(QKeyEvent *key_event)
{
    if (key_event->type() == QEvent::KeyPress)
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        if (key_event->key() == Qt::Key_Escape)
        {
            if (m_py_object.isValid())
            {
                if (app->dispatchPyMethod(m_py_object, "escapePressed", QVariantList()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
        else if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter)
        {
            if (m_py_object.isValid())
            {
                if (app->dispatchPyMethod(m_py_object, "returnPressed", QVariantList()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
        else
        {
            if (m_py_object.isValid())
            {
                Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
                if (app->dispatchPyMethod(m_py_object, "keyPressed", QVariantList() << key_event->text() << key_event->key() << (int)key_event->modifiers()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
    }

    QTextEdit::keyPressEvent(key_event);
}

void PyTextBrowser::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusIn", QVariantList());
    }

    QTextEdit::focusInEvent(event);
}

void PyTextBrowser::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusOut", QVariantList());
    }

    QTextEdit::focusOutEvent(event);
}

void PyTextBrowser::anchorClicked(const QUrl &link)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "anchorClicked", QVariantList() << link);
    }
}

PyTextEdit::PyTextEdit()
{
    setAcceptRichText(false);
    setUndoRedoEnabled(true);
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    connect(this, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(this, SIGNAL(textChanged()), this, SLOT(textChanged()));
}

void PyTextEdit::cursorPositionChanged()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "cursorPositionChanged", QVariantList());
    }
}

void PyTextEdit::selectionChanged()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "selectionChanged", QVariantList());
    }
}

void PyTextEdit::textChanged()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "textChanged", QVariantList());
    }
}

void PyTextEdit::keyPressEvent(QKeyEvent *key_event)
{
    if (key_event->type() == QEvent::KeyPress)
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        if (key_event->key() == Qt::Key_Escape)
        {
            if (m_py_object.isValid())
            {
                if (app->dispatchPyMethod(m_py_object, "escapePressed", QVariantList()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
        else if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter)
        {
            if (m_py_object.isValid())
            {
                if (app->dispatchPyMethod(m_py_object, "returnPressed", QVariantList()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
        else
        {
            if (m_py_object.isValid())
            {
                Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
                if (app->dispatchPyMethod(m_py_object, "keyPressed", QVariantList() << key_event->text() << key_event->key() << (int)key_event->modifiers()).toBool())
                {
                    key_event->accept();
                    return;
                }
            }
        }
    }

    QTextEdit::keyPressEvent(key_event);
}

void PyTextEdit::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusIn", QVariantList());
    }

    QTextEdit::focusInEvent(event);
}

void PyTextEdit::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusOut", QVariantList());
    }

    QTextEdit::focusOutEvent(event);
}

void PyTextEdit::insertFromMimeData(const QMimeData *mime_data)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

        QVariantList args;

        args << QVariant::fromValue((QObject *)mime_data);

        app->dispatchPyMethod(m_py_object, "insertFromMimeData", args);
    }
}


Overlay::Overlay(QWidget *parent, QWidget *child)
    : QWidget(parent)
    , m_child(child)
{
    parent->installEventFilter(this);

    setPalette(Qt::transparent);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    if (m_child)
    {
        m_child->setPalette(Qt::transparent);
        m_child->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_child->setParent(this);
    }
}

bool Overlay::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize && obj == parent())
    {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
        resize(resizeEvent->size());
    }
    return QWidget::eventFilter(obj, event);
}

void Overlay::resizeEvent(QResizeEvent *event)
{
    if (m_child)
        m_child->resize(event->size());
    QWidget::resizeEvent(event);
}

PyScrollArea::PyScrollArea()
{
    setWidgetResizable(true);  // do not set this, otherwise appearance of scroll bars reduces viewport size

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setAlignment(Qt::AlignCenter);

    viewport()->installEventFilter(this); // make sure we detect initial resize

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollBarChanged(int)));
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollBarChanged(int)));
}

bool PyScrollArea::eventFilter(QObject *obj, QEvent *event)
{
    bool result = QScrollArea::eventFilter(obj, event);
    if (event->type() == QEvent::Resize && obj == viewport())
    {
        notifyViewportChanged();
    }
    return result;
}

void PyScrollArea::notifyViewportChanged()
{
    if (m_py_object.isValid())
    {
        float display_scaling = GetDisplayScaling();

        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        QPoint offset = widget()->mapFrom(viewport(), QPoint(0, 0));
        QRect viewport_rect = viewport()->rect().translated(offset.x(), offset.y());
        app->dispatchPyMethod(m_py_object, "viewportChanged", QVariantList() << int(viewport_rect.left() / display_scaling) << int(viewport_rect.top() / display_scaling) << int(viewport_rect.width() / display_scaling) << int(viewport_rect.height() / display_scaling));
    }
}

void PyScrollArea::scrollBarChanged(int value)
{
    Q_UNUSED(value)

    notifyViewportChanged();
}

void PyScrollArea::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusIn", QVariantList());
    }

    QScrollArea::focusInEvent(event);
}

void PyScrollArea::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusOut", QVariantList());
    }

    QScrollArea::focusOutEvent(event);
}

void PyScrollArea::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    if (m_py_object.isValid())
    {
        float display_scaling = GetDisplayScaling();

        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "sizeChanged", QVariantList() << int(event->size().width() / display_scaling) << int(event->size().height()) / display_scaling);
        notifyViewportChanged();
    }
}

PyTabWidget::PyTabWidget()
{
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
}

void PyTabWidget::currentChanged(int index)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "currentTabChanged", QVariantList() << index);
    }
}

// see http://www.mathopenref.com/coordtrianglearea.html
static inline float triangleArea(const QPointF &p1, const QPointF &p2, const QPointF &p3)
{
    return fabs(0.5 * (p1.x() * (p2.y() - p3.y()) + p2.x() * (p3.y() - p1.y()) + p3.x() * (p1.y() - p2.y())));
}

// see http://www.dbp-consulting.com/tutorials/canvas/CanvasArcTo.html
void addArcToPath(QPainterPath &path, float x, float y, float radius, float start_angle_radians, float end_angle_radians, bool counter_clockwise)
{
    // qDebug() << "arc " << x << "," << y << "," << radius << "," << start_angle_radians << "," << end_angle_radians << "," << counter_clockwise;
    double x_start = x - radius;
    double y_start = y - radius;
    double width  = radius * 2;
    double height = radius * 2;
    bool clockwise = !counter_clockwise;

    // first check if drawing more than the circumference of the circle
    if (clockwise && (end_angle_radians - start_angle_radians >= 2 * M_PI))
    {
        end_angle_radians = start_angle_radians + 2 * M_PI;
    }
    else if (!clockwise && (start_angle_radians - end_angle_radians >= 2 * M_PI))
    {
        start_angle_radians = end_angle_radians - 2 * M_PI;
    }

    // on canvas, angles and sweep_length are in degrees clockwise from positive x-axis
    // in Qt, angles are counter-clockwise from positive x-axis; position sweep_length draws counter-clockwise
    // calculate accordingly.

    double start_angle_degrees = -180 * start_angle_radians / M_PI;
    double end_angle_degrees = -180 * end_angle_radians / M_PI;

    double sweep_angle_degrees = 0.0;

    if (clockwise)
    {
        // clockwise from 10 to 20 (canvas) => -10 to -20 (qt) => -10 + -10 (qt)
        // clockwise from -20 to -10 (canvas) => 20 to 10 (qt) => 20 + -10 (qt)
        // clockwise from 10 to -20 (canvas) => -10 to 20 (qt) => -10 to 340 => -10 - 330 (qt)
        // remember, degrees have already been negated here, i.e. in qt degrees.
        if (start_angle_degrees < end_angle_degrees)
            sweep_angle_degrees = end_angle_degrees - start_angle_degrees - 360.0;
        else
            sweep_angle_degrees = end_angle_degrees - start_angle_degrees;
    }
    else
    {
        // counterclockwise from 20 to 10 (canvas) => -20 to -10 (qt) => -20 + 10 (qt)
        // counterclockwise from -20 to -10 (canvas) => 20 to 10 (qt) => 20 + 350 (qt)
        // counterclockwise from 10 to -20 (canvas) => -10 to 20 (qt) => -10 + 30 (qt)
        // remember, degrees have already been negated here, i.e. in qt degrees.
        if (end_angle_degrees < start_angle_degrees)
            sweep_angle_degrees = end_angle_degrees - start_angle_degrees + 360.0;
        else
            sweep_angle_degrees = end_angle_degrees - start_angle_degrees;
    }

    if (radius == 0.0)
    {
        // just draw the center point
        path.lineTo(x, y);
    }
    else
    {
        // arcTo angle is counter-clockwise from positive x-axis; position sweep_length draws counter-clockwise
        path.arcTo(x_start, y_start, width, height, start_angle_degrees, sweep_angle_degrees);
    }
}

struct DrawingContextState
{
    QColor fill_color;
    int fill_gradient;
    QColor line_color;
    float line_width;
    float line_dash;
    Qt::PenCapStyle line_cap;
    Qt::PenJoinStyle line_join;
    QFont text_font;
    int text_baseline;
    int text_align;
    QMap<int, QGradient> gradients;
    QPainterPath path;
    float context_scaling_x;
    float context_scaling_y;
};

void PaintCommands(QPainter &painter, const QList<CanvasDrawingCommand> &commands, PaintImageCache *image_cache, float display_scaling)
{
    QPainterPath path;

    display_scaling = display_scaling ? display_scaling : GetDisplayScaling();

    if (image_cache)
    {
        Q_FOREACH(int image_id, image_cache->keys())
        {
            PaintImageCacheEntry &entry = (*image_cache)[image_id];
            entry.used = false;
        }
    }

    QColor fill_color(Qt::transparent);
    int fill_gradient = -1;

    QColor line_color(Qt::black);
    float line_width = 1.0;
    float line_dash = 0.0;
    Qt::PenCapStyle line_cap = Qt::SquareCap;
    Qt::PenJoinStyle line_join = Qt::BevelJoin;

    QFont text_font;
    int text_baseline = 4; // alphabetic
    int text_align = 1; // start

    float context_scaling_x = 1.0;
    float context_scaling_y = 1.0;

    QMap<int, QGradient> gradients;

    painter.fillRect(painter.viewport(), QBrush(fill_color));

    //qDebug() << "BEGIN";

    QList<DrawingContextState> stack;

    Q_FOREACH(const CanvasDrawingCommand &command, commands)
    {
        QVariantList args = command.arguments;
        QString cmd = command.command;

        //qDebug() << cmd << ": " << args;

        if (cmd == "save")
        {
            DrawingContextState values;
            values.fill_color = fill_color;
            values.fill_gradient = fill_gradient;
            values.line_color = line_color;
            values.line_width = line_width;
            values.line_dash = line_dash;
            values.line_cap = line_cap;
            values.line_join = line_join;
            values.text_font = text_font;
            values.text_baseline = text_baseline;
            values.text_align = text_align;
            values.gradients = gradients;
            values.path = path;
            values.context_scaling_x = context_scaling_x;
            values.context_scaling_y = context_scaling_y;
            stack.push_back(values);
            painter.save();
            break;
        }
        else if (cmd == "restore")
        {
            DrawingContextState values = stack.takeLast();
            fill_color = values.fill_color;
            fill_gradient = values.fill_gradient;
            line_color = values.line_color;
            line_width = values.line_width;
            line_dash = values.line_dash;
            line_cap = values.line_cap;
            line_join = values.line_join;
            text_font = values.text_font;
            text_baseline = values.text_baseline;
            text_align = values.text_align;
            gradients = values.gradients;
            path = values.path;
            context_scaling_x = values.context_scaling_x;
            context_scaling_y = values.context_scaling_y;
            painter.restore();
            break;
        }
        else if (cmd == "beginPath")
        {
            path = QPainterPath();
        }
        else if (cmd == "closePath")
        {
            path.closeSubpath();
        }
        else if (cmd == "clip")
        {
            painter.setClipRect(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling, args[2].toFloat() * display_scaling, args[3].toFloat() * display_scaling, Qt::IntersectClip);
        }
        else if (cmd == "translate")
        {
            painter.translate(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling);
        }
        else if (cmd == "scale")
        {
            painter.scale(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling);
            context_scaling_x *= args[0].toFloat();
            context_scaling_y *= args[1].toFloat();
        }
        else if (cmd == "rotate")
        {
            painter.rotate(args[0].toFloat());
        }
        else if (cmd == "moveTo")
        {
            path.moveTo(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling);
        }
        else if (cmd == "lineTo")
        {
            path.lineTo(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling);
        }
        else if (cmd == "rect")
        {
            path.addRect(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling, args[2].toFloat() * display_scaling, args[3].toFloat() * display_scaling);
        }
        else if (cmd == "arc")
        {
            // see http://www.w3.org/TR/2dcontext/#dom-context-2d-arc
            // see https://qt.gitorious.org/qt/qtdeclarative/source/e3eba2902fcf645bf88764f5272e2987e8992cd4:src/quick/items/context2d/qquickcontext2d.cpp#L3801-3815

            float x = args[0].toFloat() * display_scaling;
            float y = args[1].toFloat() * display_scaling;
            float radius = args[2].toFloat() * display_scaling;
            float start_angle_radians = args[3].toFloat();
            float end_angle_radians = args[4].toFloat();
            bool clockwise = !args[5].toBool();

            addArcToPath(path, x, y, radius, start_angle_radians, end_angle_radians, !clockwise);
        }
        else if (cmd == "arcTo")
        {
            // see https://github.com/WebKit/webkit/blob/master/Source/WebCore/platform/graphics/cairo/PathCairo.cpp
            // see https://code.google.com/p/chromium/codesearch#chromium/src/third_party/skia/src/core/SkPath.cpp&sq=package:chromium&type=cs&l=1381&rcl=1424120049
            // see https://bug-23003-attachments.webkit.org/attachment.cgi?id=26267

            QPointF p0 = path.currentPosition();
            QPointF p1(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling);
            QPointF p2(args[2].toFloat() * display_scaling, args[3].toFloat() * display_scaling);
            float radius = args[4].toFloat() * display_scaling;

            // Draw only a straight line to p1 if any of the points are equal or the radius is zero
            // or the points are collinear (triangle that the points form has area of zero value).
            if ((p1 == p0) || (p1 == p2) || radius == 0.0 || triangleArea(p0, p1, p2) == 0.0)
            {
                // just draw a line
                path.lineTo(p1.x(), p1.y());
                return;
            }

            QPointF p1p0 = p0 - p1;
            QPointF p1p2 = p2 - p1;
            float p1p0_length = sqrtf(p1p0.x() * p1p0.x() + p1p0.y() * p1p0.y());
            float p1p2_length = sqrtf(p1p2.x() * p1p2.x() + p1p2.y() * p1p2.y());

            double cos_phi = (p1p0.x() * p1p2.x() + p1p0.y() * p1p2.y()) / (p1p0_length * p1p2_length);
            // all points on a line logic
            if (cos_phi == -1) {
                path.lineTo(p1.x(), p1.y());
                return;
            }
            if (cos_phi == 1) {
                // add infinite far away point
                unsigned int max_length = 65535;
                double factor_max = max_length / p1p0_length;
                QPointF ep((p0.x() + factor_max * p1p0.x()), (p0.y() + factor_max * p1p0.y()));
                path.lineTo(ep.x(), ep.y());
                return;
            }

            float tangent = radius / tanf(acosf(cos_phi) / 2);
            float factor_p1p0 = tangent / p1p0_length;
            QPointF t_p1p0 = p1 + factor_p1p0 * p1p0;

            QPointF orth_p1p0(p1p0.y(), -p1p0.x());
            float orth_p1p0_length = sqrt(orth_p1p0.x() * orth_p1p0.x() + orth_p1p0.y() * orth_p1p0.y());
            float factor_ra = radius / orth_p1p0_length;

            // angle between orth_p1p0 and p1p2 to get the right vector orthographic to p1p0
            double cos_alpha = (orth_p1p0.x() * p1p2.x() + orth_p1p0.y() * p1p2.y()) / (orth_p1p0_length * p1p2_length);
            if (cos_alpha < 0.f)
                orth_p1p0 = QPointF(-orth_p1p0.x(), -orth_p1p0.y());

            QPointF p = t_p1p0 + factor_ra * orth_p1p0;

            // calculate angles for addArc
            orth_p1p0 = QPointF(-orth_p1p0.x(), -orth_p1p0.y());
            float sa = acosf(orth_p1p0.x() / orth_p1p0_length);
            if (orth_p1p0.y() < 0.f)
                sa = 2 * M_PI - sa;

            // anticlockwise logic
            bool anticlockwise = false;

            float factor_p1p2 = tangent / p1p2_length;
            QPointF t_p1p2 = p1 + factor_p1p2 * p1p2;
            QPointF orth_p1p2 = t_p1p2 - p;
            float orth_p1p2_length = sqrtf(orth_p1p2.x() * orth_p1p2.x() + orth_p1p2.y() * orth_p1p2.y());
            float ea = acosf(orth_p1p2.x() / orth_p1p2_length);
            if (orth_p1p2.y() < 0) {
                ea = 2 * M_PI - ea;
            }
            if ((sa > ea) && ((sa - ea) < M_PI))
                anticlockwise = true;
            if ((sa < ea) && ((ea - sa) > M_PI))
                anticlockwise = true;

            path.lineTo(t_p1p0.x(), t_p1p0.y());

            addArcToPath(path, p.x(), p.y(), radius, sa, ea, anticlockwise);
        }
        else if (cmd == "cubicTo")
        {
            path.cubicTo(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling, args[2].toFloat() * display_scaling, args[3].toFloat() * display_scaling, args[4].toFloat() * display_scaling, args[5].toFloat() * display_scaling);
        }
        else if (cmd == "quadraticTo")
        {
            path.quadTo(args[0].toFloat() * display_scaling, args[1].toFloat() * display_scaling, args[2].toFloat() * display_scaling, args[3].toFloat() * display_scaling);
        }
        else if (cmd == "statistics")
        {
            QString label = args[0].toString().simplified();

            static QMap<QString, QElapsedTimer> timer_map;
            static QMap<QString, QQueue<float> > times_map;
            static QMap<QString, unsigned> count_map;

            if (!timer_map.contains(label))
                timer_map.insert(label, QElapsedTimer());
            if (!times_map.contains(label))
                times_map.insert(label, QQueue<float>());
            if (!count_map.contains(label))
                count_map.insert(label, 0);

            QElapsedTimer &timer = timer_map[label];
            QQueue<float> &times = times_map[label];
            unsigned &count = count_map[label];

            if (timer.isValid())
            {
                times.enqueue(timer.elapsed() / 1000.0);
                while (times.length() > 50)
                    times.dequeue();

                count += 1;
                if (count == 50)
                {
                    float sum = 0.0;
                    float mn = 9999.0;
                    float mx = 0.0;
                    Q_FOREACH(float time, times)
                    {
                        sum += time;
                        mn = qMin(mn, time);
                        mx = qMax(mx, time);
                    }
                    float mean = sum / times.length();
                    float sum_of_squares = 0.0;
                    Q_FOREACH(float time, times)
                    {
                        sum_of_squares += (time - mean) * (time - mean);
                    }
                    float std_dev = sqrt(sum_of_squares / times.length());
                    qDebug() << label << " fps " << int(100 * (1.0 / mean))/100.0 << " mean " << mean << " dev " << std_dev << " min " << mn << " max " << mx;
                    count = 0;
                }
            }

            timer.restart();
        }
        else if (cmd == "image")
        {
            int width = args[1].toInt();
            int height = args[2].toInt();

            int image_id = args[3].toInt();

            if (image_cache && image_cache->contains(image_id))
            {
                (*image_cache)[image_id].used = true;
                QImage image = (*image_cache)[image_id].image;
                painter.drawImage(QRectF(QPointF(args[4].toFloat() * display_scaling, args[5].toFloat() * display_scaling), QSizeF(args[6].toFloat() * display_scaling, args[7].toFloat() * display_scaling)), image);
            }
            else
            {
                QImageInterface image;

                QRectF destination_rect(QPointF(args[4].toFloat() * display_scaling, args[5].toFloat() * display_scaling), QSizeF(args[6].toFloat() * display_scaling, args[7].toFloat() * display_scaling));
                float context_scaling = qMin(context_scaling_x, context_scaling_y);
                QSize destination_size((destination_rect.size() * context_scaling).toSize());

                {
                    Python_ThreadBlock thread_block;

                    // Grab the ndarray
                    PyObjectPtr ndarray_py(QVariantToPyObject(args[2]));
                    if (ndarray_py)
                    {
                        PythonSupport::instance()->imageFromRGBA(ndarray_py, &image);
                    }
                }

                if (!image.image.isNull())
                {
                    if (destination_size.width() < width * 0.75 || destination_size.height() < height * 0.75)
                    {
                        image.image = image.image.scaled((destination_rect.size() * context_scaling).toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    painter.drawImage(destination_rect, image.image);
                    if (image_cache)
                    {
                        PaintImageCacheEntry cache_entry(image_id, true, image.image);
                        (*image_cache)[image_id] = cache_entry;
                    }
                }
            }
        }
        else if (cmd == "data")
        {
            int image_id = args[3].toInt();

            if (image_cache && image_cache->contains(image_id))
            {
                (*image_cache)[image_id].used = true;
                QImage image = (*image_cache)[image_id].image;
                painter.drawImage(QRectF(QPointF(args[4].toFloat() * display_scaling, args[5].toFloat() * display_scaling), QSizeF(args[6].toFloat() * display_scaling, args[7].toFloat() * display_scaling)), image);
            }
            else
            {
                QImageInterface image;

                QRectF destination_rect(QPointF(args[4].toFloat() * display_scaling, args[5].toFloat() * display_scaling), QSizeF(args[6].toFloat() * display_scaling, args[7].toFloat() * display_scaling));
                float context_scaling = qMin(context_scaling_x, context_scaling_y);

                {
                    Python_ThreadBlock thread_block;

                    // Grab the ndarray
                    PyObjectPtr ndarray_py(QVariantToPyObject(args[2]));

                    if (ndarray_py)
                    {
                        PyObject *colormap_ndarray_py = NULL;

                        if (args[10].toInt() != 0)
                            colormap_ndarray_py = QVariantToPyObject(args[10]);

                        PythonSupport::instance()->scaledImageFromArray(ndarray_py, destination_rect.width(), destination_rect.height(), context_scaling, args[8].toFloat(), args[9].toFloat(), colormap_ndarray_py, &image);
                    }
                }

                if (!image.image.isNull())
                {
                    painter.drawImage(destination_rect, image.image);
                    if (image_cache)
                    {
                        PaintImageCacheEntry cache_entry(image_id, true, image.image);
                        (*image_cache)[image_id] = cache_entry;
                    }
                }
            }
        }
        else if (cmd == "stroke")
        {
            QPen pen(line_color);
            pen.setWidthF(line_width * display_scaling);
            pen.setJoinStyle(line_join);
            pen.setCapStyle(line_cap);
            if (line_dash > 0.0)
            {
                QVector<qreal> dashes;
                dashes << line_dash * display_scaling << line_dash * display_scaling;
                pen.setDashPattern(dashes);
            }
            painter.strokePath(path, pen);
        }
        else if (cmd == "fill")
        {
            QBrush brush = fill_gradient >= 0 ? QBrush(gradients[fill_gradient]) : QBrush(fill_color);
            painter.fillPath(path, brush);
        }
        else if (cmd == "fillStyle")
        {
            QString color_arg = args[0].toString().simplified();
            fill_color = ParseColorString(color_arg);
            fill_gradient = -1;
        }
        else if (cmd == "fillStyleGradient")
        {
            fill_gradient = args[0].toInt();
        }
        else if (cmd == "fillText" || cmd == "strokeText")
        {
            QString text = args[0].toString();
            QPointF text_pos(args[1].toFloat() * display_scaling, args[2].toFloat() * display_scaling);
            QFontMetrics fm(text_font);
            int text_width = fm.horizontalAdvance(text);
            if (text_align == 2 || text_align == 5) // end or right
                text_pos.setX(text_pos.x() - text_width);
            else if (text_align == 4) // center
                text_pos.setX(text_pos.x() - text_width*0.5);
            if (text_baseline == 1)    // top
                text_pos.setY(text_pos.y() + fm.ascent());
            else if (text_baseline == 2)    // hanging
                text_pos.setY(text_pos.y() + 2 * fm.ascent() - fm.height());
            else if (text_baseline == 3)    // middle
                text_pos.setY(text_pos.y() + fm.xHeight() * 0.5);
            else if (text_baseline == 4 || text_baseline == 5)  // alphabetic or ideographic
                text_pos.setY(text_pos.y());
            else if (text_baseline == 5)    // bottom
                text_pos.setY(text_pos.y() + fm.ascent() - fm.height());
            QPainterPath path;
            path.addText(text_pos, text_font, text);
            if (cmd == "fillText")
            {
                QBrush brush = fill_gradient >= 0 ? QBrush(gradients[fill_gradient]) : QBrush(fill_color);
                painter.fillPath(path, brush);
            }
            else
            {
                QPen pen(line_color);
                pen.setWidth(line_width * display_scaling);
                pen.setJoinStyle(line_join);
                pen.setCapStyle(line_cap);
                painter.strokePath(path, pen);
            }
        }
        else if (cmd == "font")
        {
            text_font = ParseFontString(args[0].toString(), display_scaling);
        }
        else if (cmd == "textAlign")
        {
            if (args[0].toString() == "start")
                text_align = 1;
            if (args[0].toString() == "end")
                text_align = 2;
            if (args[0].toString() == "left")
                text_align = 3;
            if (args[0].toString() == "center")
                text_align = 4;
            if (args[0].toString() == "right")
                text_align = 5;
        }
        else if (cmd == "textBaseline")
        {
            if (args[0].toString() == "top")
                text_baseline = 1;
            if (args[0].toString() == "hanging")
                text_baseline = 2;
            if (args[0].toString() == "middle")
                text_baseline = 3;
            if (args[0].toString() == "alphabetic")
                text_baseline = 4;
            if (args[0].toString() == "ideographic")
                text_baseline = 5;
            if (args[0].toString() == "bottom")
                text_baseline = 6;
        }
        else if (cmd == "strokeStyle")
        {
            QString color_arg = args[0].toString().simplified();
            line_color = ParseColorString(color_arg);
        }
        else if (cmd == "lineDash")
        {
            line_dash = args[0].toFloat();
        }
        else if (cmd == "lineWidth")
        {
            line_width = args[0].toFloat();
        }
        else if (cmd == "lineCap")
        {
            if (args[0].toString() == "square")
                line_cap = Qt::SquareCap;
            if (args[0].toString() == "round")
                line_cap = Qt::RoundCap;
            if (args[0].toString() == "butt")
                line_cap = Qt::FlatCap;
        }
        else if (cmd == "lineJoin")
        {
            if (args[0].toString() == "round")
                line_join = Qt::RoundJoin;
            if (args[0].toString() == "miter")
                line_join = Qt::MiterJoin;
            if (args[0].toString() == "bevel")
                line_join = Qt::BevelJoin;
        }
        else if (cmd == "gradient")
        {
            gradients[args[0].toInt()] = QLinearGradient(args[3].toFloat() * display_scaling, args[4].toFloat() * display_scaling, args[3].toFloat() * display_scaling + args[5].toFloat() * display_scaling, args[4].toFloat() * display_scaling + args[6].toFloat() * display_scaling);
        }
        else if (cmd == "colorStop")
        {
            gradients[args[0].toInt()].setColorAt(args[1].toFloat(), QColor(args[2].toString()));
        }
        else if (cmd == "sleep")
        {
            unsigned long duration = args[0].toFloat() * 1000000L;
            QThread::usleep(duration);
        }
        else if (cmd == "latency")
        {
            extern QElapsedTimer timer;
            extern qint64 timer_offset_ns;
            qDebug() << "Latency " << qint64((timer.nsecsElapsed() - (args[0].toDouble() * 1E9 - timer_offset_ns)) / 1.0E6) << "ms";
        }
        else if (cmd == "message")
        {
            qDebug() << args[0].toString();
        }
        else if (cmd == "timestamp")
        {
        }
        else if (cmd == "begin_layer")
        {
        }
        else if (cmd == "end_layer")
        {
        }
        else if (cmd == "draw_layer")
        {
        }
    }

    if (image_cache)
    {
        Q_FOREACH(int image_id, image_cache->keys())
        {
            if (!(*image_cache)[image_id].used)
            {
                image_cache->remove(image_id);
            }
        }
    }
}

inline quint32 read_uint32(const quint32 *commands, unsigned int &command_index)
{
    return commands[command_index++];
}

inline qint32 read_int32(const quint32 *commands, unsigned int &command_index)
{
    return *(qint32 *)(&commands[command_index++]);
}

inline float read_float(const quint32 *commands, unsigned int &command_index)
{
    return *(float *)(&commands[command_index++]);
}

inline double read_double(const quint32 *commands, unsigned int &command_index)
{
    return *(double *)(&commands[command_index]);
    command_index += 2;
}

inline bool read_bool(const quint32 *commands, unsigned int &command_index)
{
    return *(quint32 *)(&commands[command_index++]) != 0;
}

inline QString read_string(const quint32 *commands, unsigned int &command_index)
{
    quint32 str_len = read_uint32(commands, command_index);
    QString str = QString::fromUtf8((const char *)&commands[command_index], str_len);
    command_index += ((str_len + 3) & 0xFFFFFFFC) / 4;
    return str;
}

struct NullDeleter {template<typename T> void operator()(T*) {} };

RenderedTimeStamps PaintBinaryCommands(QPainter *rawPainter, const std::vector<quint32> commands_v, const QMap<QString, QVariant> &imageMap, PaintImageCache *image_cache, LayerCache *layer_cache, const RenderedTimeStamps &lastRenderedTimestamps, float display_scaling, int section_id, float devicePixelRatio)
{
    QSharedPointer<QPainter> painter(rawPainter, NullDeleter());

    RenderedTimeStamps rendered_timestamps;

    // this will keep track of the total scaling applied in nested layers. it is used to
    // update the rendered_timestamps with the proper global transform, which will be drawn
    // at the top level and should not have resolution transforms applied.
    float transform_scaling = 1.0;

    display_scaling = display_scaling ? display_scaling : GetDisplayScaling();

    QPainterPath path;

    if (image_cache)
    {
        Q_FOREACH(int image_id, image_cache->keys())
        {
            PaintImageCacheEntry &entry = (*image_cache)[image_id];
            entry.used = false;
        }
    }

    QColor fill_color(Qt::transparent);
    int fill_gradient = -1;

    QColor line_color(Qt::black);
    float line_width = 1.0;
    float line_dash = 0.0;
    Qt::PenCapStyle line_cap = Qt::SquareCap;
    Qt::PenJoinStyle line_join = Qt::BevelJoin;

    QFont text_font;
    int text_baseline = 4; // alphabetic
    int text_align = 1; // start

    float context_scaling_x = 1.0;
    float context_scaling_y = 1.0;

    QMap<int, QGradient> gradients;

    painter->fillRect(painter->viewport(), QBrush(fill_color));

    QList<DrawingContextState> stack;

    QSet<int> layers_used;
    bool layer_skip = false;
    QSharedPointer<QImage> layer_image;
    QList<QSharedPointer<QPainter> > painter_stack;
    QList<QSharedPointer<QImage> > layer_image_stack;
    QList<bool> layer_skip_stack;

    unsigned int command_index = 0;

    const quint32 *commands = &commands_v[0];

    extern QElapsedTimer timer;
    extern qint64 timer_offset_ns;

    while (command_index < commands_v.size())
    {
        quint32 cmd_hex = read_uint32(commands, command_index);
        quint32 cmd = (cmd_hex & 0x000000FF) << 24 |
                      (cmd_hex & 0x0000FF00) << 8 |
                      (cmd_hex & 0x00FF0000) >> 8 |
                      (cmd_hex & 0xFF000000) >> 24;

        // qint64 start = qint64(timer.nsecsElapsed() / 1.0E3);

        if (layer_skip && cmd != 0x656e6c79 && cmd != 0x62676c79)
            continue;

        switch (cmd)
        {
            case 0x73617665:  // save
            {
                DrawingContextState values;
                values.fill_color = fill_color;
                values.fill_gradient = fill_gradient;
                values.line_color = line_color;
                values.line_width = line_width;
                values.line_dash = line_dash;
                values.line_cap = line_cap;
                values.line_join = line_join;
                values.text_font = text_font;
                values.text_baseline = text_baseline;
                values.text_align = text_align;
                values.gradients = gradients;
                values.path = path;
                values.context_scaling_x = context_scaling_x;
                values.context_scaling_y = context_scaling_y;
                stack.push_back(values);
                painter->save();
                break;
            }
            case 0x72657374:  // rest, restore
            {
                DrawingContextState values = stack.takeLast();
                fill_color = values.fill_color;
                fill_gradient = values.fill_gradient;
                line_color = values.line_color;
                line_width = values.line_width;
                line_dash = values.line_dash;
                line_cap = values.line_cap;
                line_join = values.line_join;
                text_font = values.text_font;
                text_baseline = values.text_baseline;
                text_align = values.text_align;
                gradients = values.gradients;
                path = values.path;
                context_scaling_x = values.context_scaling_x;
                context_scaling_y = values.context_scaling_y;
                painter->restore();
                break;
            }
            case 0x62707468: // bpth, begin path
            {
                path = QPainterPath();
                break;
            }
            case 0x63707468: // cpth, close path
            {
                path.closeSubpath();
                break;
            }
            case 0x636c6970: // clip
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                float a2 = read_float(commands, command_index) * display_scaling;
                float a3 = read_float(commands, command_index) * display_scaling;
                painter->setClipRect(a0, a1, a2, a3, Qt::IntersectClip);
                break;
            }
            case 0x7472616e: // tran, translate
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                painter->translate(a0, a1);
                break;
            }
            case 0x7363616c: // scal, scale
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                painter->scale(a0, a1);
                context_scaling_x *= a0;
                context_scaling_y *= a1;
                break;
            }
            case 0x726f7461: // rota, rotate
            {
                float a0 = read_float(commands, command_index);
                painter->rotate(a0);
                break;
            }
            case 0x6d6f7665: // move
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                path.moveTo(a0, a1);
                break;
            }
            case 0x6c696e65: // line
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                path.lineTo(a0, a1);
                break;
            }
            case 0x72656374: // rect
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                float a2 = read_float(commands, command_index) * display_scaling;
                float a3 = read_float(commands, command_index) * display_scaling;
                path.addRect(a0, a1, a2, a3);
                break;
            }
            case 0x61726320: // arc
            {
                // see http://www.w3.org/TR/2dcontext/#dom-context-2d-arc
                // see https://qt.gitorious.org/qt/qtdeclarative/source/e3eba2902fcf645bf88764f5272e2987e8992cd4:src/quick/items/context2d/qquickcontext2d.cpp#L3801-3815

                float x = read_float(commands, command_index) * display_scaling;
                float y = read_float(commands, command_index) * display_scaling;
                float radius = read_float(commands, command_index) * display_scaling;
                float start_angle_radians = read_float(commands, command_index);
                float end_angle_radians = read_float(commands, command_index);
                bool clockwise = !read_bool(commands, command_index);

                addArcToPath(path, x, y, radius, start_angle_radians, end_angle_radians, !clockwise);
                break;
            }
            case 0x61726374: // arct, arc to
            {
                // see https://github.com/WebKit/webkit/blob/master/Source/WebCore/platform/graphics/cairo/PathCairo.cpp
                // see https://code.google.com/p/chromium/codesearch#chromium/src/third_party/skia/src/core/SkPath.cpp&sq=package:chromium&type=cs&l=1381&rcl=1424120049
                // see https://bug-23003-attachments.webkit.org/attachment.cgi?id=26267

                QPointF p0 = path.currentPosition();
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                float a2 = read_float(commands, command_index) * display_scaling;
                float a3 = read_float(commands, command_index) * display_scaling;
                QPointF p1(a0, a1);
                QPointF p2(a2, a3);
                float radius = read_float(commands, command_index) * display_scaling;

                // Draw only a straight line to p1 if any of the points are equal or the radius is zero
                // or the points are collinear (triangle that the points form has area of zero value).
                if ((p1 == p0) || (p1 == p2) || radius == 0.0 || triangleArea(p0, p1, p2) == 0.0)
                {
                    // just draw a line
                    path.lineTo(p1.x(), p1.y());
                    // return;
                }

                QPointF p1p0 = p0 - p1;
                QPointF p1p2 = p2 - p1;
                float p1p0_length = sqrtf(p1p0.x() * p1p0.x() + p1p0.y() * p1p0.y());
                float p1p2_length = sqrtf(p1p2.x() * p1p2.x() + p1p2.y() * p1p2.y());

                double cos_phi = (p1p0.x() * p1p2.x() + p1p0.y() * p1p2.y()) / (p1p0_length * p1p2_length);
                // all points on a line logic
                if (cos_phi == -1) {
                    path.lineTo(p1.x(), p1.y());
                    // return;
                }
                if (cos_phi == 1) {
                    // add infinite far away point
                    unsigned int max_length = 65535;
                    double factor_max = max_length / p1p0_length;
                    QPointF ep((p0.x() + factor_max * p1p0.x()), (p0.y() + factor_max * p1p0.y()));
                    path.lineTo(ep.x(), ep.y());
                    // return;
                }

                float tangent = radius / tanf(acosf(cos_phi) / 2);
                float factor_p1p0 = tangent / p1p0_length;
                QPointF t_p1p0 = p1 + factor_p1p0 * p1p0;

                QPointF orth_p1p0(p1p0.y(), -p1p0.x());
                float orth_p1p0_length = sqrt(orth_p1p0.x() * orth_p1p0.x() + orth_p1p0.y() * orth_p1p0.y());
                float factor_ra = radius / orth_p1p0_length;

                // angle between orth_p1p0 and p1p2 to get the right vector orthographic to p1p0
                double cos_alpha = (orth_p1p0.x() * p1p2.x() + orth_p1p0.y() * p1p2.y()) / (orth_p1p0_length * p1p2_length);
                if (cos_alpha < 0.f)
                    orth_p1p0 = QPointF(-orth_p1p0.x(), -orth_p1p0.y());

                QPointF p = t_p1p0 + factor_ra * orth_p1p0;

                // calculate angles for addArc
                orth_p1p0 = QPointF(-orth_p1p0.x(), -orth_p1p0.y());
                float sa = acosf(orth_p1p0.x() / orth_p1p0_length);
                if (orth_p1p0.y() < 0.f)
                    sa = 2 * M_PI - sa;

                // anticlockwise logic
                bool anticlockwise = false;

                float factor_p1p2 = tangent / p1p2_length;
                QPointF t_p1p2 = p1 + factor_p1p2 * p1p2;
                QPointF orth_p1p2 = t_p1p2 - p;
                float orth_p1p2_length = sqrtf(orth_p1p2.x() * orth_p1p2.x() + orth_p1p2.y() * orth_p1p2.y());
                float ea = acosf(orth_p1p2.x() / orth_p1p2_length);
                if (orth_p1p2.y() < 0) {
                    ea = 2 * M_PI - ea;
                }
                if ((sa > ea) && ((sa - ea) < M_PI))
                    anticlockwise = true;
                if ((sa < ea) && ((ea - sa) > M_PI))
                    anticlockwise = true;

                path.lineTo(t_p1p0.x(), t_p1p0.y());

                addArcToPath(path, p.x(), p.y(), radius, sa, ea, anticlockwise);
                break;
            }
            case 0x63756263: // cubc, cubic to
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                float a2 = read_float(commands, command_index) * display_scaling;
                float a3 = read_float(commands, command_index) * display_scaling;
                float a4 = read_float(commands, command_index) * display_scaling;
                float a5 = read_float(commands, command_index) * display_scaling;
                path.cubicTo(a0, a1, a2, a3, a4, a5);
                break;
            }
            case 0x71756164: // quad, quadratic to
            {
                float a0 = read_float(commands, command_index) * display_scaling;
                float a1 = read_float(commands, command_index) * display_scaling;
                float a2 = read_float(commands, command_index) * display_scaling;
                float a3 = read_float(commands, command_index) * display_scaling;
                path.quadTo(a0, a1, a2, a3);
                break;
            }
            case 0x73746174: // stat, statistics
            {
                QString label = read_string(commands, command_index).simplified();

                static QMap<QString, QElapsedTimer> timer_map;
                static QMap<QString, QQueue<float> > times_map;
                static QMap<QString, unsigned> count_map;

                if (!timer_map.contains(label))
                    timer_map.insert(label, QElapsedTimer());
                if (!times_map.contains(label))
                    times_map.insert(label, QQueue<float>());
                if (!count_map.contains(label))
                    count_map.insert(label, 0);

                QElapsedTimer &timer = timer_map[label];
                QQueue<float> &times = times_map[label];
                unsigned &count = count_map[label];

                if (timer.isValid())
                {
                    times.enqueue(timer.elapsed() / 1000.0);
                    while (times.length() > 50)
                        times.dequeue();

                    count += 1;
                    if (count == 50)
                    {
                        float sum = 0.0;
                        float mn = 9999.0;
                        float mx = 0.0;
                        Q_FOREACH(float time, times)
                        {
                            sum += time;
                            mn = qMin(mn, time);
                            mx = qMax(mx, time);
                        }
                        float mean = sum / times.length();
                        float sum_of_squares = 0.0;
                        Q_FOREACH(float time, times)
                        {
                            sum_of_squares += (time - mean) * (time - mean);
                        }
                        float std_dev = sqrt(sum_of_squares / times.length());
                        qDebug() << label << " fps " << int(100 * (1.0 / mean)) / 100.0 << " mean " << mean << " dev " << std_dev << " min " << mn << " max " << mx;
                        count = 0;
                    }
                }

                timer.restart();
                break;
            }
            case 0x696d6167: // imag, image
            {
                int width = read_uint32(commands, command_index); // width
                int height = read_uint32(commands, command_index); // height

                int image_id = read_uint32(commands, command_index);

                // std::cout << "display scaling " << display_scaling << " devicePixelRatio " << devicePixelRatio << std::endl;

                float arg4 = read_float(commands, command_index) * display_scaling;
                float arg5 = read_float(commands, command_index) * display_scaling;
                float arg6 = read_float(commands, command_index) * display_scaling;
                float arg7 = read_float(commands, command_index) * display_scaling;

                if (image_cache && image_cache->contains(image_id))
                {
                    (*image_cache)[image_id].used = true;
                    QImage image = (*image_cache)[image_id].image;
                    painter->drawImage(QRectF(QPointF(arg4, arg5), QSizeF(arg6, arg7)), image);
                }
                else
                {
                    // QElapsedTimer timer;
                    // timer.start();

                    QImageInterface image;

                    QRectF destination_rect(QPointF(arg4, arg5), QSizeF(arg6, arg7));
                    float context_scaling = qMin(context_scaling_x, context_scaling_y);
                    QSize destination_size((destination_rect.size() * context_scaling).toSize());
                    QSize device_destination_size = destination_size * devicePixelRatio;

                    QString image_key = QString::number(image_id);

                    if (imageMap.contains(image_key))
                    {
                        Python_ThreadBlock thread_block;

                        // Put the ndarray in image
                        PyObjectPtr ndarray_py(QVariantToPyObject(imageMap[image_key]));
                        if (ndarray_py)
                        {
                            // scaledImageFromRGBA is slower than using image.scaled.
                            // image = PythonSupport::instance()->scaledImageFromRGBA(ndarray_py, destination_size);
                            PythonSupport::instance()->imageFromRGBA(ndarray_py, &image);
                        }
                        // std::cout << "Using cached image" << std::endl;
                    }
                    else
                        qDebug() << "missing " << image_key;

                    if (!image.image.isNull())
                    {
                        if (device_destination_size.width() < width * 0.75 || device_destination_size.height() < height * 0.75)
                        {
                            image.image = image.image.scaled(device_destination_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        }
                        painter->drawImage(destination_rect, image.image);
                        if (image_cache)
                        {
                            PaintImageCacheEntry cache_entry(image_id, true, image.image);
                            (*image_cache)[image_id] = cache_entry;
                        }
                    }

                    // std::cout << "Elapsed: " << timer.elapsed() << "ms " << width << "x" << height << " ; " << image.width() << "x" << image.height() << " ; " << destination_size.width() << "x" << destination_size.height() << std::endl;
                }

                break;
            }
            case 0x64617461: // data, image data
            {
                read_uint32(commands, command_index); // width
                read_uint32(commands, command_index); // height
//                int width = read_uint32(commands, command_index); // width
//                int height = read_uint32(commands, command_index); // height

                int image_id = read_uint32(commands, command_index);

                float arg4 = read_float(commands, command_index) * display_scaling;
                float arg5 = read_float(commands, command_index) * display_scaling;
                float arg6 = read_float(commands, command_index) * display_scaling;
                float arg7 = read_float(commands, command_index) * display_scaling;

                float low = read_float(commands, command_index);
                float high = read_float(commands, command_index);

                int color_map_image_id = read_uint32(commands, command_index);

                if (image_cache && image_cache->contains(image_id))
                {
                    (*image_cache)[image_id].used = true;
                    QImage image = (*image_cache)[image_id].image;
                    painter->drawImage(QRectF(QPointF(arg4, arg5), QSizeF(arg6, arg7)), image);
                }
                else
                {
//                    QTime timer;
//                    timer.start();

                    QImageInterface image;

                    QRectF destination_rect(QPointF(arg4, arg5), QSizeF(arg6, arg7));
                    float context_scaling = qMin(context_scaling_x, context_scaling_y);
                    QSize destination_size((destination_rect.size()* context_scaling).toSize());
                    QSize device_destination_size = destination_size * devicePixelRatio;

                    QString image_key = QString::number(image_id);

                    if (imageMap.contains(image_key))
                    {
                        Python_ThreadBlock thread_block;

                        // Put the ndarray in image
                        PyObjectPtr ndarray_py(QVariantToPyObject(imageMap[image_key]));
                        if (ndarray_py)
                        {
                            PyObject *colormap_ndarray_py = NULL;

                            if (color_map_image_id != 0)
                            {
                                QString color_map_image_key = QString::number(color_map_image_id);
                                if (imageMap.contains(color_map_image_key))
                                    colormap_ndarray_py = (PyObject *)QVariantToPyObject(imageMap[color_map_image_key]);
                            }

//                          PythonSupport::instance()->imageFromArray(ndarray_py, low, high, colormap_ndarray_py, &image);
                            PythonSupport::instance()->scaledImageFromArray(ndarray_py, device_destination_size.width(), device_destination_size.height(), context_scaling, low, high, colormap_ndarray_py, &image);
                        }
                    }
                    else
                        qDebug() << "missing " << image_key;

                    if (!image.image.isNull())
                    {
                        painter->drawImage(destination_rect, image.image);
                        if (image_cache)
                        {
                            PaintImageCacheEntry cache_entry(image_id, true, image.image);
                            (*image_cache)[image_id] = cache_entry;
                        }
                    }

//                    qDebug() << "Elapsed: " << timer.elapsed() << "ms " << width << "x" << height << " ; " << image.width() << "x" << image.height() << " ; " << int(destination_rect.width() * context_scaling) << "x" << int(destination_rect.height() * context_scaling);
                }
                break;
            }
            case 0x7374726b: // strk, stroke
            {
                QPen pen(line_color);
                pen.setWidthF(line_width * display_scaling);
                pen.setJoinStyle(line_join);
                pen.setCapStyle(line_cap);
                if (line_dash > 0.0)
                {
                    QVector<qreal> dashes;
                    dashes << line_dash * display_scaling << line_dash * display_scaling;
                    pen.setDashPattern(dashes);
                }
                painter->strokePath(path, pen);
                break;
            }
            case 0x66696c6c: // fill
            {
                QBrush brush = fill_gradient >= 0 ? QBrush(gradients[fill_gradient]) : QBrush(fill_color);
                painter->fillPath(path, brush);
                break;
            }
            case 0x666c7374: // flst, fill style
            {
                QString color_arg = read_string(commands, command_index).simplified();
                fill_color = ParseColorString(color_arg);
                fill_gradient = -1;
                break;
            }
            case 0x666c7367: // flsg, fill style gradient
            {
                fill_gradient = read_uint32(commands, command_index);
                break;
            }
            case 0x74657874:
            case 0x73747874: // text, stxt; fill text, stroke text
            {
                QString text = read_string(commands, command_index);
                float arg1 = read_float(commands, command_index) * display_scaling;
                float arg2 = read_float(commands, command_index) * display_scaling;
                read_float(commands, command_index); // max width
                QPointF text_pos(arg1, arg2);
                QFontMetrics fm(text_font);
                int text_width = fm.horizontalAdvance(text);
                if (text_align == 2 || text_align == 5) // end or right
                    text_pos.setX(text_pos.x() - text_width);
                else if (text_align == 4) // center
                    text_pos.setX(text_pos.x() - text_width * 0.5);
                if (text_baseline == 1)    // top
                    text_pos.setY(text_pos.y() + fm.ascent());
                else if (text_baseline == 2)    // hanging
                    text_pos.setY(text_pos.y() + 2 * fm.ascent() - fm.height());
                else if (text_baseline == 3)    // middle
                    text_pos.setY(text_pos.y() + fm.xHeight() * 0.5);
                else if (text_baseline == 4 || text_baseline == 5)  // alphabetic or ideographic
                    text_pos.setY(text_pos.y());
                else if (text_baseline == 5)    // bottom
                    text_pos.setY(text_pos.y() + fm.ascent() - fm.height());
                if (cmd == 0x74657874) // text, fill text
                {
                    QBrush brush = fill_gradient >= 0 ? QBrush(gradients[fill_gradient]) : QBrush(fill_color);
                    painter->save();
                    painter->setFont(text_font);
                    painter->setPen(QPen(brush, 1.0 * display_scaling));
                    painter->drawText(text_pos, text);
                    painter->restore();
                }
                else // stroke text
                {
                    QPainterPath path;
                    path.addText(text_pos, text_font, text);
                    QPen pen(line_color);
                    pen.setWidth(line_width * display_scaling);
                    pen.setJoinStyle(line_join);
                    pen.setCapStyle(line_cap);
                    painter->strokePath(path, pen);
                }
                break;
            }
            case 0x666f6e74: // font
            {
                QString font_str = read_string(commands, command_index);
                text_font = ParseFontString(font_str, display_scaling);
                break;
            }
            case 0x616c676e: // algn, text align
            {
                QString arg0 = read_string(commands, command_index);
                if (arg0 == "start")
                    text_align = 1;
                if (arg0 == "end")
                    text_align = 2;
                if (arg0 == "left")
                    text_align = 3;
                if (arg0 == "center")
                    text_align = 4;
                if (arg0 == "right")
                    text_align = 5;
                break;
            }
            case 0x74626173: // tbas, textBaseline
            {
                QString arg0 = read_string(commands, command_index);
                if (arg0 == "top")
                    text_baseline = 1;
                if (arg0 == "hanging")
                    text_baseline = 2;
                if (arg0 == "middle")
                    text_baseline = 3;
                if (arg0 == "alphabetic")
                    text_baseline = 4;
                if (arg0 == "ideographic")
                    text_baseline = 5;
                if (arg0 == "bottom")
                    text_baseline = 6;
                break;
            }
            case 0x73747374: // stst, strokeStyle
            {
                QString arg0 = read_string(commands, command_index);
                QString color_arg = arg0.simplified();
                line_color = ParseColorString(color_arg);
                break;
            }
            case 0x6c647368: // ldsh, line dash
            {
                line_dash = read_float(commands, command_index);
                break;
            }
            case 0x6c696e77: // linw, lineWidth
            {
                line_width = read_float(commands, command_index);
                break;
            }
            case 0x6c636170: // lcap, lineCap
            {
                QString arg0 = read_string(commands, command_index);
                if (arg0 == "square")
                    line_cap = Qt::SquareCap;
                if (arg0 == "round")
                    line_cap = Qt::RoundCap;
                if (arg0 == "butt")
                    line_cap = Qt::FlatCap;
                break;
            }
            case 0x6c6e6a6e: // lnjn, lineJoin
            {
                QString arg0 = read_string(commands, command_index);
                if (arg0 == "round")
                    line_join = Qt::RoundJoin;
                if (arg0 == "miter")
                    line_join = Qt::MiterJoin;
                if (arg0 == "bevel")
                    line_join = Qt::BevelJoin;
                break;
            }
            case 0x67726164: // grad, gradient
            {
                int arg0 = read_uint32(commands, command_index);
                read_float(commands, command_index);
                read_float(commands, command_index);
                float arg3 = read_float(commands, command_index) * display_scaling;
                float arg4 = read_float(commands, command_index) * display_scaling;
                float arg5 = read_float(commands, command_index) * display_scaling;
                float arg6 = read_float(commands, command_index) * display_scaling;
                gradients[arg0] = QLinearGradient(arg3, arg4, arg3 + arg5, arg4 + arg6);
                break;
            }
            case 0x67726373: // grcs, colorStop
            {
                int arg0 = read_uint32(commands, command_index);
                float arg1 = read_float(commands, command_index);
                QString arg2 = read_string(commands, command_index);
                gradients[arg0].setColorAt(arg1, QColor(arg2));
                break;
            }
            case 0x736c6570: // slep, sleep
            {
                unsigned long duration = read_float(commands, command_index) * 1000000L;
                QThread::usleep(duration);
                break;
            }
            case 0x6c61746e: // latn, latency
            {
                double arg0 = read_double(commands, command_index);
                qDebug() << "Latency " << qint64((timer.nsecsElapsed() - ((double)arg0 * 1E9 - timer_offset_ns)) / 1.0E6) << "ms";
                break;
            }
            case 0x6d657367: // mesg, message
            {
                qDebug() << read_string(commands, command_index);
                break;
            }
            case 0x74696d65: // time, message
            {
                QString text = read_string(commands, command_index);
                int64_t timestamp_ns = 0;
                int64_t elapsed_ns = 0;
                if (text.length() > 4)
                {
                    // calculate new date time
                    timestamp_ns = text.toULongLong();
                }
                else
                {
                    // use existing date time, elapsedDuration
                    Q_FOREACH(const RenderedTimeStamp &rendered_timestamp, lastRenderedTimestamps)
                    {
                        if (rendered_timestamp.section_id == section_id)
                        {
                            timestamp_ns = rendered_timestamp.time_stamp_ns;
                            elapsed_ns = rendered_timestamp.elapsed_ns;
                            text = rendered_timestamp.text;
                        }
                    }
                }
                painter->save();
                QPointF text_pos(12, 12);
                QFont text_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
                QFontMetrics fm(text_font);
                int text_width = fm.horizontalAdvance(text);
                int text_ascent = fm.ascent();
                int text_height = fm.height();
                QPainterPath background;
                background.addRect(text_pos.x() - 4, text_pos.y() - 4, text_width + 8, text_height + 8);
                painter->fillPath(background, Qt::white);
                QPainterPath path;
                path.addText(text_pos.x(), text_pos.y() + text_ascent, text_font, text);
                painter->fillPath(path, Qt::black);
                painter->restore();
                QTransform transform = painter->transform();
                QList<QSharedPointer<QPainter> > painter_stack_reversed = painter_stack;
                std::reverse(painter_stack.begin(), painter_stack.end());
                Q_FOREACH(QSharedPointer<QPainter> p, painter_stack_reversed)
                {
                    transform = QTransform::fromScale(1/transform_scaling, 1/transform_scaling) * p->transform() * transform;
                }
                rendered_timestamps.append(RenderedTimeStamp(transform, timestamp_ns, section_id, elapsed_ns, text));
                break;
            }
            case 0x62676c79: // begin layer
            {
                quint32 layer_id = read_uint32(commands, command_index);
                quint32 layer_seed = read_uint32(commands, command_index);
                float layer_rect_top = read_float(commands, command_index) * display_scaling;
                float layer_rect_left = read_float(commands, command_index) * display_scaling;
                float layer_rect_height = read_float(commands, command_index) * display_scaling;
                float layer_rect_width = read_float(commands, command_index) * display_scaling;
                QRect layer_rect((int)layer_rect_left, (int)layer_rect_top, (int)layer_rect_width, (int)layer_rect_height);
                layer_skip_stack.push_back(layer_skip);
                if (!layer_skip)
                {
                    if (layer_cache->contains(layer_id) && layer_seed == layer_cache->value(layer_id).layer_seed)
                    {
                        layer_skip = true;
                    }
                    else
                    {
                        painter_stack.push_back(painter);
                        layer_image_stack.push_back(layer_image);
                        // create the layer image at a resolution suitable for the devicePixelRatio of the section's screen.
                        layer_image = QSharedPointer<QImage>(new QImage(QSize(layer_rect.width() * devicePixelRatio, layer_rect.height() * devicePixelRatio), QImage::Format_ARGB32_Premultiplied));
                        layer_image->fill(QColor(0,0,0,0));
                        painter = QSharedPointer<QPainter>(new QPainter(layer_image.data()));
                        painter->setRenderHints(DEFAULT_RENDER_HINTS);
                        // draw everything at the higher scale of the section's screen.
                        painter->scale(devicePixelRatio, devicePixelRatio);
                        // track the transform scaling
                        transform_scaling *= devicePixelRatio;
                        painter->translate(layer_rect_left, layer_rect_top);
                    }
                }
                layers_used.insert(layer_id);
                break;
            }
            case 0x656e6c79: // end layer
            {
                quint32 layer_id = read_uint32(commands, command_index);
                quint32 layer_seed = read_uint32(commands, command_index);
                float layer_rect_top = read_float(commands, command_index) * display_scaling;
                float layer_rect_left = read_float(commands, command_index) * display_scaling;
                float layer_rect_height = read_float(commands, command_index) * display_scaling;
                float layer_rect_width = read_float(commands, command_index) * display_scaling;
                QRect layer_rect((int)layer_rect_left, (int)layer_rect_top, (int)layer_rect_width, (int)layer_rect_height);
                layer_skip = layer_skip_stack.takeLast();
                if (!layer_skip)
                {
                    if (layer_cache->contains(layer_id) && layer_seed == layer_cache->value(layer_id).layer_seed)
                    {
                        layer_skip = false;
                        QSharedPointer<QImage> layer_image = layer_cache->value(layer_id).layer_image;
                        layer_rect = layer_cache->value(layer_id).layer_rect;
                        painter->drawImage(layer_rect, *layer_image);
                    }
                    else
                    {
                        painter->end();
                        layer_cache->insert(layer_id, LayerCacheEntry(layer_seed, layer_image, layer_rect));
                        painter = painter_stack.takeLast();
                        painter->drawImage(layer_rect, *layer_image);
                        layer_image = layer_image_stack.takeLast();
                        // track the transform scaling
                        transform_scaling /= devicePixelRatio;
                    }
                }
                break;
            }
        }

        // qint64 end = qint64(timer.nsecsElapsed() / 1.0E3);
        // if (end - start > 50)
        //     qDebug() << "cmd " << QString::number(cmd, 16) << " " << (end - start);
    }

    if (image_cache)
    {
        Q_FOREACH(int image_id, image_cache->keys())
        {
            if (!image_cache->value(image_id).used)
            {
                image_cache->remove(image_id);
            }
        }
    }

    if (layer_cache)
    {
        Q_FOREACH(int layer_id, layer_cache->keys())
        {
            if (!layers_used.contains(layer_id))
            {
                layer_cache->take(layer_id);
            }
        }
    }

    return rendered_timestamps;
}

PyCanvasRenderTask::PyCanvasRenderTask(PyCanvas *canvas)
    : m_canvas(canvas)
    , m_signals(new PyCanvasRenderTaskSignals())
{
}

PyCanvasRenderTask::~PyCanvasRenderTask()
{
    delete m_signals;
}

void PyCanvasRenderTask::run()
{
    QRectOptional repaint_rect = m_canvas->renderOne();
    if (repaint_rect.has_value)
        Q_EMIT m_signals->renderingReady(repaint_rect.value);
}

PyCanvas::PyCanvas()
    : m_pressed(false)
    , m_grab_mouse_count(0)
    , m_rendering_count(0)
{
    setMouseTracking(true);
    setAcceptDrops(true);

    m_timer.start();
}

PyCanvas::~PyCanvas()
{
    QMutexLocker locker(&m_rendering_count_mutex);
    while (m_rendering_count > 0)
    {
        m_rendering_count_mutex.unlock();
        QThread::msleep(1);
        m_rendering_count_mutex.lock();
    }
}

void PyCanvas::repaintRect(const QRect &repaintRect)
{
    update(repaintRect);
}

void PyCanvas::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusIn", QVariantList());
    }

    QWidget::focusInEvent(event);
}

void PyCanvas::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusOut", QVariantList());
    }

    QWidget::focusOutEvent(event);
}

class RenderCounter
{
    QMutex *m;
    int &rendering_count;
public:
    RenderCounter(QMutex *m, int &rendering_count) : m(m), rendering_count(rendering_count)
    {
        QMutexLocker locker(m);
        rendering_count += 1;
    }
    ~RenderCounter()
    {
        QMutexLocker locker(m);
        rendering_count -= 1;
    }
};

QRectOptional PyCanvas::renderOne()
{
    RenderCounter render_counter(&m_rendering_count_mutex, m_rendering_count);

    QList<QSharedPointer<CanvasSection> > sections;

    {
        QMutexLocker locker(&m_commands_mutex);
        sections = m_sections.values();
    }

    QSharedPointer<CanvasSection> nextSection;
    Q_FOREACH(QSharedPointer<CanvasSection> section, sections)
    {
        QMutexLocker locker(&section->m_mutex);
        // first check whether the section can be rendered (not rendering already and has commands to render)
        if (!section->rendering && !section->m_commands_binary.empty())
        {
            // next check whether it is earlier than the current next_section
            // if so, make this the new next section
            if (!nextSection || section->time < nextSection->time)
                nextSection = section;
        }
    }

    if (nextSection)
    {
        {
            QMutexLocker locker(&nextSection->m_mutex);
            // mark this section as being rendered, but check to make sure it's not being rendered
            // on another thread (avoids race condition). also check to see if it was deleted.
            if (!nextSection->rendering && !nextSection->m_commands_binary.empty())
                nextSection->rendering = true;
            else
                return QRectOptional();
        }
        QRectOptional rect_optional = renderSection(nextSection);
        {
            QMutexLocker locker(&nextSection->m_mutex);
            // mark this section as being finished. no race condition. just clear it and update the time.
            nextSection->rendering = false;
            nextSection->time = m_timer.nsecsElapsed();
        }
        wakeRenderer();
        return rect_optional;
    }

    return QRectOptional();
}

QRectOptional PyCanvas::renderSection(QSharedPointer<CanvasSection> section)
{
    std::vector<quint32> commands_binary;
    QRect rect;
    QMap<QString, QVariant> imageMap;
    int section_id;
    {
        QMutexLocker locker(&section->m_mutex);
        commands_binary = section->m_commands_binary;
        rect = section->rect;
        imageMap = section->m_imageMap;
        section_id = section->m_section_id;
        section->m_commands_binary.clear();
    }
    if (!commands_binary.empty() && !rect.isEmpty())
    {
        float devicePixelRatio = section->m_device_pixel_ratio;
        // create the buffer image at a resolution suitable for the devicePixelRatio of the section's screen.
        QSharedPointer<QImage> image = QSharedPointer<QImage>(new QImage(QSize(rect.width() * devicePixelRatio, rect.height() * devicePixelRatio), QImage::Format_ARGB32_Premultiplied));
        image->fill(QColor(0,0,0,0));
        QPainter painter(image.data());
        painter.setRenderHints(DEFAULT_RENDER_HINTS);
        // draw everything at the higher scale of the section's screen.
        painter.scale(devicePixelRatio, devicePixelRatio);
        section->last_rendered_timestamps = PaintBinaryCommands(&painter, commands_binary, imageMap, &section->m_image_cache, &section->m_layer_cache, section->last_rendered_timestamps, 0.0, section_id, devicePixelRatio);
        painter.end();  // ending painter here speeds up QImage assignment below (Windows)

        QMutexLocker locker(&section->m_mutex);
        section->image = image;
        section->image_rect = rect;
        section->m_rendered_timestamps.clear();
        Q_FOREACH(RenderedTimeStamp r, section->last_rendered_timestamps)
        {
            QTransform transform = r.transform;
            transform.translate(rect.left(), rect.top());
            transform = transform * QTransform::fromScale(1/devicePixelRatio, 1/devicePixelRatio);
            section->m_rendered_timestamps.append(RenderedTimeStamp(transform, r.time_stamp_ns, r.section_id));
        }
        section->record_latency = true;
        return QRectOptional(rect);
    }
    return QRectOptional();
}

void PyCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter;

    painter.begin(this);

    QList<QSharedPointer<CanvasSection> > sections;

    {
        QMutexLocker locker(&m_commands_mutex);
        sections = m_sections.values();
    }

    RenderedTimeStamps rendered_timestamps;

    Q_FOREACH(QSharedPointer<CanvasSection> section, sections)
    {
        QSharedPointer<QImage> image;
        QRect image_rect;
        {
            QMutexLocker locker(&section->m_mutex);
            image = section->image;
            image_rect = section->image_rect;

#if defined(__APPLE__)
            int64_t current_time_ns = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#endif
#if defined(_WIN32) || defined(_WIN64)
            auto current_time_point = std::chrono::high_resolution_clock::now();
            int64_t current_time_ns = current_time_point.time_since_epoch().count();
#endif
#if defined(__linux__)
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            int64_t current_time_ns = static_cast<int64_t>(ts.tv_nsec);
#endif

            for (RenderedTimeStamps::iterator i = section->m_rendered_timestamps.begin(); i != section->m_rendered_timestamps.end(); ++i)
            {
                RenderedTimeStamp &rendered_timestamp = *i;
                if (!rendered_timestamp.elapsed_ns)
                {
                    rendered_timestamp.elapsed_ns = current_time_ns - rendered_timestamp.time_stamp_ns;
                }
            }
            rendered_timestamps.append(section->m_rendered_timestamps);
        }
        // qDebug() << "paintEvent " << image->isNull() << " " << image_rect << " " << event->rect();
        if (image && !image->isNull() && image_rect.intersects(event->rect()))
        {
            // qDebug() << "draw " << image_rect.topLeft();
            painter.drawImage(image_rect, *image);
        }
    }

    Q_FOREACH(const RenderedTimeStamp &rendered_timestamp, rendered_timestamps)
    {
        painter.save();
        painter.setRenderHints(DEFAULT_RENDER_HINTS);
        int64_t latency_min_ns = 1000000000;
        int64_t latency_avg_ns = 0;
        int64_t latency_max_ns = 0;
        double latency_std_ns = 0;
        QString latencyListString;
        if (rendered_timestamp.section_id > 0)
        {
            QSharedPointer<CanvasSection> section;
            {
                QMutexLocker locker(&m_commands_mutex);
                section = m_sections[rendered_timestamp.section_id];
            }
            QMutexLocker locker(&section->latenciesMutex);
            if (section->record_latency)
            {
                section->latencies_ns.enqueue(rendered_timestamp.elapsed_ns);
                while (section->latencies_ns.size() > 40)
                    section->latencies_ns.dequeue();
                section->record_latency = false;
            }
            auto latencies_ns(section->latencies_ns);
            std::sort(latencies_ns.begin(), latencies_ns.end());
            int discard = latencies_ns.size() / 10;
            while (discard > 0)
            {
                if (latencies_ns.size())
                    latencies_ns.pop_front();
                discard -= 1;
            }
            Q_FOREACH(quint64 latency_ns, latencies_ns)
            {
                latency_avg_ns += latency_ns;
                if (latency_ns < latency_min_ns)
                    latency_min_ns = latency_ns;
                if (latency_ns > latency_max_ns)
                    latency_max_ns = latency_ns;
            }
            double latencyAverageF = (double)latency_avg_ns / latencies_ns.size();
            latency_avg_ns = (int64_t)latencyAverageF;
            double sumSquares = 0;
            Q_FOREACH(quint64 latency_ns, latencies_ns)
            {
                sumSquares += (latency_ns - latencyAverageF) * (latency_ns - latencyAverageF);
                latencyListString += " " + QString::number(qRound(latency_ns / 1e6));
            }
            latency_std_ns = sqrt(sumSquares / latencies_ns.size() );
        }
        QString text = "Latency " + QString::number(static_cast<int>(qRound(rendered_timestamp.elapsed_ns / 1e6))).rightJustified(4);
        if (latency_avg_ns > 0)
            text += ":" + QString::number(qRound(latency_avg_ns / 1e6)).rightJustified(3) + " ± " + QString::number(latency_std_ns / 1e6, 'f', 1).rightJustified(4) + " [" + QString::number(qRound(latency_min_ns / 1e6)).rightJustified(3) + ":" + QString::number(qRound(latency_max_ns / 1e6)).rightJustified(3) + " ] ";
        QFont text_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        QFontMetrics fm(text_font);
        int text_width = fm.horizontalAdvance(text);
        int text_ascent = fm.ascent();
        int text_height = fm.height();
        QPointF text_pos(12, 12 + text_height + 16);
        QTransform world_transform = rendered_timestamp.transform;
        painter.setWorldTransform(world_transform);
        QPainterPath background;
        background.addRect(text_pos.x() - 4, text_pos.y() - 4, text_width + 8, text_height + 8);
        painter.fillPath(background, Qt::white);
        QPainterPath path;
        path.addText(text_pos.x(), text_pos.y() + text_ascent, text_font, text);
        painter.fillPath(path, Qt::black);
        painter.restore();
    }
}

bool PyCanvas::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::Gesture:
        {
            QGestureEvent *gesture_event = static_cast<QGestureEvent *>(event);
            QPanGesture *pan_gesture = static_cast<QPanGesture *>(gesture_event->gesture(Qt::PanGesture));
            if (pan_gesture)
            {
                Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
                float display_scaling = GetDisplayScaling();
                if (app->dispatchPyMethod(m_py_object, "panGesture", QVariantList() << int(pan_gesture->delta().x() / display_scaling) << int(pan_gesture->delta().y() / display_scaling)).toBool())
                    return true;
            }
            QPinchGesture *pinch_gesture = static_cast<QPinchGesture *>(gesture_event->gesture(Qt::PinchGesture));
            if (pinch_gesture)
            {
                qDebug() << "pinch";
            }
        } break;
        case QEvent::ToolTip:
        {
            Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
            QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
            float display_scaling = GetDisplayScaling();
            if (app->dispatchPyMethod(m_py_object, "helpEvent", QVariantList() << int(helpEvent->pos().x() / display_scaling) << int(helpEvent->pos().y() / display_scaling) << int(helpEvent->globalPos().x() / display_scaling) << int(helpEvent->globalPos().y() / display_scaling)).toBool())
                return true;
        } break;
        default: break;
    }
    return QWidget::event(event);
}

void PyCanvas::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mouseEntered", QVariantList());
    }
}

void PyCanvas::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mouseExited", QVariantList());
    }
}

void PyCanvas::mousePressEvent(QMouseEvent *event)
{
    if (m_py_object.isValid() && event->button() == Qt::LeftButton)
    {
        float display_scaling = GetDisplayScaling();

        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mousePressed", QVariantList() << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling) << (int)event->modifiers());
        m_last_pos = event->pos();
        m_pressed = true;
    }
}

void PyCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_py_object.isValid() && event->button() == Qt::LeftButton)
    {
        float display_scaling = GetDisplayScaling();

        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mouseReleased", QVariantList() << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling) << (int)event->modifiers());
        m_pressed = false;

        if ((event->pos() - m_last_pos).manhattanLength() < 6 * display_scaling)
        {
            app->dispatchPyMethod(m_py_object, "mouseClicked", QVariantList() << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling) << (int)event->modifiers());
        }
    }
}

void PyCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_py_object.isValid() && event->button() == Qt::LeftButton)
    {
        float display_scaling = GetDisplayScaling();

        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mouseDoubleClicked", QVariantList() << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling) << (int)event->modifiers());
    }
}

void PyCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

        float display_scaling = GetDisplayScaling();

        if (m_grab_mouse_count > 0)
        {
            QPoint delta = event->pos() - m_grab_reference_point;

            app->dispatchPyMethod(m_py_object, "grabbedMousePositionChanged", QVariantList() << int(delta.x() / display_scaling) << int(delta.y() / display_scaling) << (int)event->modifiers());

            QCursor::setPos(mapToGlobal(m_grab_reference_point));
            QApplication::changeOverrideCursor(Qt::BlankCursor);
        }

        app->dispatchPyMethod(m_py_object, "mousePositionChanged", QVariantList() << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling) << (int)event->modifiers());

        // handle case of not getting mouse released event after drag.
        if (m_pressed && !(event->buttons() & Qt::LeftButton))
        {
            app->dispatchPyMethod(m_py_object, "mouseReleased", QVariantList() << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling) << (int)event->modifiers());
            m_pressed = false;
        }
    }
}

void PyCanvas::wheelEvent(QWheelEvent *event)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        QWheelEvent *wheel_event = static_cast<QWheelEvent *>(event);
        float display_scaling = GetDisplayScaling();
        bool is_horizontal = abs(wheel_event->angleDelta().rx()) > abs(wheel_event->angleDelta().ry());
        QPoint delta = wheel_event->pixelDelta().isNull() ? wheel_event->angleDelta() : wheel_event->pixelDelta();
        app->dispatchPyMethod(m_py_object, "wheelChanged", QVariantList() << int(wheel_event->position().x() / display_scaling) << int(wheel_event->position().y() / display_scaling) << int(delta.x() / display_scaling) << int(delta.y() / display_scaling) << (bool)is_horizontal);
    }
}

void PyCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_py_object.isValid())
    {
        float display_scaling = GetDisplayScaling();

        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "sizeChanged", QVariantList() << int(event->size().width() / display_scaling) << int(event->size().height()) / display_scaling);
    }
}

void PyCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        if (m_py_object.isValid())
        {
            Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
            if (app->dispatchPyMethod(m_py_object, "keyPressed", QVariantList() << event->text() << event->key() << (int)event->modifiers()).toBool())
            {
                event->accept();
                return;
            }
        }
    }

    QWidget::keyPressEvent(event);
}

void PyCanvas::keyReleaseEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyRelease)
    {
        if (m_py_object.isValid())
        {
            Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
            if (app->dispatchPyMethod(m_py_object, "keyReleased", QVariantList() << event->text() << event->key() << (int)event->modifiers()).toBool())
            {
                event->accept();
                return;
            }
        }
    }

    QWidget::keyReleaseEvent(event);
}

void PyCanvas::contextMenuEvent(QContextMenuEvent *event)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariantList args;

    float display_scaling = GetDisplayScaling();

    args.push_back(int(event->pos().x() / display_scaling));
    args.push_back(int(event->pos().y() / display_scaling));
    args.push_back(int(event->globalPos().x() / display_scaling));
    args.push_back(int(event->globalPos().y() / display_scaling));

    app->dispatchPyMethod(m_py_object, "contextMenuEvent", args);
}

void PyCanvas::grabMouse0(const QPoint &gp)
{
    unsigned grab_mouse_count = m_grab_mouse_count;
    m_grab_mouse_count += 1;
    if (grab_mouse_count == 0)
    {
        grabMouse();
        grabKeyboard();
        m_grab_reference_point = gp;
        QCursor::setPos(gp);
        QApplication::setOverrideCursor(Qt::BlankCursor);
    }
}

void PyCanvas::releaseMouse0()
{
    m_grab_mouse_count -= 1;
    if (m_grab_mouse_count == 0)
    {
        releaseMouse();
        releaseKeyboard();
        QApplication::restoreOverrideCursor();
    }
}

void PyCanvas::renderingFinished()
{
    QTimer::singleShot(0, this, SLOT(update()));
}

void PyCanvas::setCommands(const QList<CanvasDrawingCommand> &commands)
{
    {
        QMutexLocker locker(&m_commands_mutex);
        m_commands = commands;
    }

    wakeRenderer();
}

void PyCanvas::setBinaryCommands(const std::vector<quint32> &commands, const QMap<QString, QVariant> &imageMap)
{
    setBinarySectionCommands(0, commands, rect(), imageMap);
}

void PyCanvas::setBinarySectionCommands(int section_id, const std::vector<quint32> &commands, const QRect &rect, const QMap<QString, QVariant> &imageMap)
{
    QSharedPointer<CanvasSection> section;

    {
        QMutexLocker locker(&m_commands_mutex);
        if (m_sections.contains(section_id))
        {
            section = m_sections[section_id];
        }
        else
        {
            QSharedPointer<CanvasSection> new_section(new CanvasSection());
            m_sections[section_id] = new_section;
            section = new_section;
            auto screen = this->screen();
            section->m_device_pixel_ratio = screen ? screen->devicePixelRatio() : 1.0;  // m_screen may be nullptr in earlier versions of Qt
            section->m_section_id = section_id;
            section->rendering = false;
            section->time = 0;
            section->record_latency = false;
        }
    }

    QMap<QString, QVariant> imageMapCopy;

    {
        QMutexLocker locker(&section->m_mutex);
        section->m_commands_binary = commands;
        imageMapCopy = section->m_imageMap;  // ensure the original gets released outside of the lock
        section->rect = rect;
        section->m_imageMap = imageMap;
    }

    wakeRenderer();
}

void PyCanvas::wakeRenderer()
{
    PyCanvasRenderTask *task = new PyCanvasRenderTask(this);
    connect(task->signals(), SIGNAL(renderingReady(const QRect &)), this, SLOT(repaintRect(const QRect &)));
    QThreadPool::globalInstance()->start(task);
}

void PyCanvas::removeSection(int section_id)
{
    QMutexLocker locker(&m_commands_mutex);
    m_sections.remove(section_id);
}

void PyCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        QString action = app->dispatchPyMethod(m_py_object, "dragEnterEvent", QVariantList() << QVariant::fromValue((QObject *)event->mimeData())).toString();
        if (action == "copy")
        {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        }
        else if (action == "move")
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else if (action == "accept")
        {
            event->accept();
        }
        else
        {
            QWidget::dragEnterEvent(event);
        }
    }
    else
    {
        QWidget::dragEnterEvent(event);
    }
}

void PyCanvas::dragLeaveEvent(QDragLeaveEvent *event)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        QString action = app->dispatchPyMethod(m_py_object, "dragLeaveEvent", QVariantList()).toString();
        if (action == "accept")
        {
            event->accept();
        }
        else
        {
            QWidget::dragLeaveEvent(event);
        }
    }
    else
    {
        QWidget::dragLeaveEvent(event);
    }
}

void PyCanvas::dragMoveEvent(QDragMoveEvent *event)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        float display_scaling = GetDisplayScaling();
        QString action = app->dispatchPyMethod(m_py_object, "dragMoveEvent", QVariantList() << QVariant::fromValue((QObject *)event->mimeData()) << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling)).toString();
        if (action == "copy")
        {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        }
        else if (action == "move")
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else if (action == "accept")
        {
            event->accept();
        }
        else
        {
            QWidget::dragMoveEvent(event);
        }
    }
    else
    {
        QWidget::dragMoveEvent(event);
    }
}

void PyCanvas::dropEvent(QDropEvent *event)
{
    QWidget::dropEvent(event);
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        float display_scaling = GetDisplayScaling();
        QString action = app->dispatchPyMethod(m_py_object, "dropEvent", QVariantList() << QVariant::fromValue((QObject *)event->mimeData()) << int(event->position().x() / display_scaling) << int(event->position().y() / display_scaling)).toString();
        if (action == "copy")
        {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        }
        else if (action == "move")
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else if (action == "accept")
        {
            event->accept();
        }
        else
        {
            QWidget::dropEvent(event);
        }
    }
    else
    {
        QWidget::dropEvent(event);
    }
}

void ApplyStylesheet(QWidget *widget)
{
    static QString stylesheet;

    if (stylesheet.isEmpty())
    {
        QFile stylesheet_file(":/app/stylesheet.qss");
        if (stylesheet_file.open(QIODevice::ReadOnly))
        {
            stylesheet = stylesheet_file.readAll();

#if defined(Q_OS_WIN)
            stylesheet = "QWidget { font-size: 11px }\n" + stylesheet;
#endif

            float display_scaling = GetDisplayScaling();

            while (true)
            {
                QRegularExpression re("(\\d+)px");
                QRegularExpressionMatch match = re.match(stylesheet);
                if (match.hasMatch())
                {
                    int new_size = int(match.captured(1).toInt() * display_scaling);
                    stylesheet.replace(match.capturedStart(0), match.capturedLength(0), QString::number(new_size) + "QZ");
                }
                else
                {
                    break;
                }
            }

            stylesheet.replace("QZ", "px");

            stylesheet_file.close();
        }
    }

    widget->setStyleSheet(stylesheet);
}

QWidget *Widget_makeIntrinsicWidget(const QString &intrinsic_id)
{
    if (intrinsic_id == "row")
    {
        QWidget *row = new QWidget();
        QHBoxLayout *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        row_layout->setSpacing(0);
        ApplyStylesheet(row);
        return row;
    }
    else if (intrinsic_id == "column")
    {
        QWidget *column = new QWidget();
        QVBoxLayout *column_layout = new QVBoxLayout(column);
        column_layout->setContentsMargins(0, 0, 0, 0);
        column_layout->setSpacing(0);
        ApplyStylesheet(column);
        return column;
    }
    else if (intrinsic_id == "tab")
    {
        PyTabWidget *group = new PyTabWidget();
        group->setTabsClosable(false);
        group->setMovable(false);
        ApplyStylesheet(group);
        return group;
    }
    else if (intrinsic_id == "stack")
    {
        QStackedWidget *stack = new QStackedWidget();
        ApplyStylesheet(stack);
        return stack;
    }
    else if (intrinsic_id == "group")
    {
        QGroupBox *group_box = new QGroupBox();
        QVBoxLayout *column_layout = new QVBoxLayout(group_box);
        column_layout->setContentsMargins(0, 0, 0, 0);
        column_layout->setSpacing(0);
        ApplyStylesheet(group_box);
        return group_box;
    }
    else if (intrinsic_id == "scrollarea")
    {
        PyScrollArea *scroll_area = new PyScrollArea();
        // Set up the system wide stylesheet
        ApplyStylesheet(scroll_area);
        return scroll_area;
    }
    else if (intrinsic_id == "splitter")
    {
        QSplitter *splitter = new QSplitter();
        splitter->setOrientation(Qt::Vertical);
        ApplyStylesheet(splitter);
        return splitter;
    }
    else if (intrinsic_id == "pushbutton")
    {
        PyPushButton *button = new PyPushButton();
        return button;
    }
    else if (intrinsic_id == "radiobutton")
    {
        PyRadioButton *button = new PyRadioButton();
        return button;
    }
    else if (intrinsic_id == "checkbox")
    {
        PyCheckBox *checkbox = new PyCheckBox();
        return checkbox;
    }
    else if (intrinsic_id == "combobox")
    {
        PyComboBox *combobox = new PyComboBox();
        return combobox;
    }
    else if (intrinsic_id == "label")
    {
        QLabel *label = new QLabel();
        return label;
    }
    else if (intrinsic_id == "slider")
    {
        PySlider *slider = new PySlider();
        return slider;
    }
    else if (intrinsic_id == "lineedit")
    {
        PyLineEdit *line_edit = new PyLineEdit();
        return line_edit;
    }
    else if (intrinsic_id == "textbrowser")
    {
        PyTextBrowser *text_browser = new PyTextBrowser();
        return text_browser;
    }
    else if (intrinsic_id == "textedit")
    {
        PyTextEdit *text_edit = new PyTextEdit();
        return text_edit;
    }
    else if (intrinsic_id == "canvas")
    {
        PyCanvas *canvas = new PyCanvas();
        return canvas;
    }
    else if (intrinsic_id == "pytree")
    {
        TreeWidget *data_view = new TreeWidget();
        data_view->setStyleSheet("QListView { border: none; }");
        data_view->setHeaderHidden(true);

        QScrollArea *scroll_area = new QScrollArea();
        scroll_area->setWidgetResizable(true);
        scroll_area->setWidget(data_view);
        scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        // Set up the system wide stylesheet
        ApplyStylesheet(scroll_area);

        QWidget *content_view = new QWidget();
        content_view->setContentsMargins(0, 0, 0, 0);
        content_view->setStyleSheet("border: none; background-color: transparent");
        QVBoxLayout *content_view_layout = new QVBoxLayout(content_view);
        content_view_layout->setContentsMargins(0, 0, 0, 0);
        content_view_layout->setSpacing(0);
        content_view_layout->addWidget(scroll_area);

        return content_view;
    }

    return NULL;
}

QVariant Widget_getWidgetProperty_(QWidget *widget, const QString &property)
{
    Q_UNUSED(widget)
    Q_UNUSED(property)

    return QVariant();
}

QSizePolicy::Policy ParseSizePolicy(const QString &policy_str, QSizePolicy::Policy policy)
{
    if (policy_str.compare("fixed", Qt::CaseInsensitive) == 0)
        return QSizePolicy::Fixed;
    if (policy_str.compare("maximum", Qt::CaseInsensitive) == 0)
        return QSizePolicy::Maximum;
    if (policy_str.compare("minimum", Qt::CaseInsensitive) == 0)
        return QSizePolicy::Minimum;
    if (policy_str.compare("preferred", Qt::CaseInsensitive) == 0)
        return QSizePolicy::Preferred;
    if (policy_str.compare("expanding", Qt::CaseInsensitive) == 0)
        return QSizePolicy::Expanding;
    if (policy_str.compare("min-expanding", Qt::CaseInsensitive) == 0)
        return QSizePolicy::MinimumExpanding;
    if (policy_str.compare("ignored", Qt::CaseInsensitive) == 0)
        return QSizePolicy::Ignored;
    return policy;
}

void Widget_setWidgetProperty_(QWidget *widget, const QString &property, const QVariant &variant)
{
    if (property == "margin")
    {
        int margin = int(variant.toInt() * GetDisplayScaling());
        widget->setContentsMargins(margin, margin, margin, margin);
    }
    else if (property == "margin-top")
    {
        int value = int(variant.toInt() * GetDisplayScaling());
        QMargins margin = widget->contentsMargins();
        margin.setTop(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "margin-left")
    {
        int value = int(variant.toInt() * GetDisplayScaling());
        QMargins margin = widget->contentsMargins();
        margin.setLeft(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "margin-bottom")
    {
        int value = int(variant.toInt() * GetDisplayScaling());
        QMargins margin = widget->contentsMargins();
        margin.setBottom(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "margin-right")
    {
        int value = int(variant.toInt() * GetDisplayScaling());
        QMargins margin = widget->contentsMargins();
        margin.setRight(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "min-width")
    {
        widget->setMinimumWidth(int(variant.toInt() * GetDisplayScaling()));
    }
    else if (property == "max-width")
    {
        widget->setMaximumWidth(int(variant.toInt() * GetDisplayScaling()));
    }
    else if (property == "min-height")
    {
        widget->setMinimumHeight(int(variant.toInt() * GetDisplayScaling()));
    }
    else if (property == "max-height")
    {
        widget->setMaximumHeight(int(variant.toInt() * GetDisplayScaling()));
    }
    else if (property == "size-policy-horizontal")
    {
        QSizePolicy size_policy = widget->sizePolicy();
        size_policy.setHorizontalPolicy(ParseSizePolicy(variant.toString(), size_policy.horizontalPolicy()));
        widget->setSizePolicy(size_policy);
    }
    else if (property == "size-policy-vertical")
    {
        QSizePolicy size_policy = widget->sizePolicy();
        size_policy.setVerticalPolicy(ParseSizePolicy(variant.toString(), size_policy.verticalPolicy()));
        widget->setSizePolicy(size_policy);
    }
    else if (property == "width")
    {
        widget->setMinimumWidth(int(variant.toInt() * GetDisplayScaling()));
        widget->setMaximumWidth(int(variant.toInt() * GetDisplayScaling()));
    }
    else if (property == "height")
    {
        widget->setMinimumHeight(int(variant.toInt() * GetDisplayScaling()));
        widget->setMaximumHeight(int(variant.toInt() * GetDisplayScaling()));
    }
    else if (property == "spacing")
    {
        QBoxLayout *layout = dynamic_cast<QBoxLayout *>(widget->layout());
        if (layout)
            layout->setSpacing(int(variant.toInt() * GetDisplayScaling()));
    }
    else if (property == "font-size")
    {
        QFont font = widget->font();
        font.setPointSize(int(variant.toInt() * GetDisplayScaling()));
        widget->setFont(font);
    }
    else if (property == "stylesheet")
    {
        widget->setStyleSheet(variant.toString());
    }
}


// -----------------------------------------------------------
// PyAction
// -----------------------------------------------------------

PyAction::PyAction(QObject *parent)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), this, SLOT(triggered()));
}

void PyAction::triggered()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "triggered", QVariantList());
    }
}

// -----------------------------------------------------------
// Drag
// -----------------------------------------------------------
Drag::Drag(QWidget *widget)
    : QDrag(widget)
{
}

void Drag::execute()
{
    Qt::DropAction action = exec(Qt::CopyAction | Qt::MoveAction);
    QMap<Qt::DropAction, QString> mapping;
    mapping[Qt::CopyAction] = "copy";
    mapping[Qt::MoveAction] = "move";
    mapping[Qt::LinkAction] = "link";
    mapping[Qt::IgnoreAction] = "ignore";
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
    app->dispatchPyMethod(m_py_object, "dragFinished", QVariantList() << mapping[action]);
}


// -----------------------------------------------------------
// PyMenu
// -----------------------------------------------------------

PyMenu::PyMenu()
{
    connect(this, SIGNAL(aboutToShow()), this, SLOT(aboutToShow()));
    connect(this, SIGNAL(aboutToHide()), this, SLOT(aboutToHide()));
}

void PyMenu::aboutToShow()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "aboutToShow", QVariantList());
    }
}

void PyMenu::aboutToHide()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "aboutToHide", QVariantList());
    }
}


// -----------------------------------------------------------
// TreeWidget
// -----------------------------------------------------------

TreeWidget::TreeWidget()
{
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragEnabled(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(clicked(QModelIndex)));
    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleClicked(QModelIndex)));
}

void TreeWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusIn", QVariantList());
    }

    QTreeView::focusInEvent(event);
}

void TreeWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusOut", QVariantList());
    }

    QTreeView::focusOutEvent(event);
}

void TreeWidget::setModelAndConnect(ItemModel *py_item_model)
{
    setModel(py_item_model);

    connect(py_item_model, SIGNAL(modelAboutToBeReset()), this, SLOT(modelAboutToBeReset()));
    connect(py_item_model, SIGNAL(modelReset()), this, SLOT(modelReset()));
}

void TreeWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress && handleKey(event->text(), event->key(), event->modifiers()))
        return;

    QTreeView::keyPressEvent(event);
}

void TreeWidget::dropEvent(QDropEvent *event)
{
    QTreeView::dropEvent(event);
    if (event->isAccepted())
    {
        ItemModel *py_item_model = dynamic_cast<ItemModel *>(model());
        event->setDropAction(py_item_model->lastDropAction());
    }
}

void TreeWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);

    int row = current.row();
    int parent_row = -1;
    int parent_id = 0;
    if (current.parent().isValid())
    {
        parent_row = current.parent().row();
        parent_id = (int)(current.parent().internalId());
    }

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    app->dispatchPyMethod(m_py_object, "treeItemChanged", QVariantList() << row << parent_row << parent_id);
}

void TreeWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    // note the parameters passed represent the CHANGES not the new and old selection

    QTreeView::selectionChanged(selected, deselected);

    QVariantList selected_indexes;

    Q_FOREACH(const QModelIndex &index, selectedIndexes())
    {
        int row = index.row();
        int parent_row = -1;
        int parent_id = 0;
        if (index.parent().isValid())
        {
            parent_row = index.parent().row();
            parent_id = (int)(index.parent().internalId());
        }

        QVariantList selected_index;

        selected_index << row << parent_row << parent_id;

        selected_indexes.push_back(selected_index);
    }

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariantList args;

    args.push_back(selected_indexes);

    app->dispatchPyMethod(m_py_object, "treeSelectionChanged", args);
}

void TreeWidget::modelAboutToBeReset()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    m_saved_index = currentIndex().row();
}

void TreeWidget::modelReset()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    setCurrentIndex(model()->index(m_saved_index, 0));
}

bool TreeWidget::handleKey(const QString &text, int key, int modifiers)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariantList selected_indexes;

    Q_FOREACH(const QModelIndex &index, selectedIndexes())
    {
        int row = index.row();
        int parent_row = -1;
        int parent_id = 0;
        if (index.parent().isValid())
        {
            parent_row = index.parent().row();
            parent_id = (int)(index.parent().internalId());
        }

        QVariantList selected_index;

        selected_index << row << parent_row << parent_id;

        selected_indexes.push_back(selected_index);
    }

    if (selected_indexes.size() == 1)
    {
        QVariantList selected_index = selected_indexes.at(0).toList();
        int row = selected_index.at(0).toInt();
        int parent_row = selected_index.at(1).toInt();
        int parent_id = selected_index.at(2).toInt();
        if (app->dispatchPyMethod(m_py_object, "treeItemKeyPressed", QVariantList() << row << parent_row << parent_id << text << key << modifiers).toBool())
            return true;
    }

    QVariantList args;

    args.push_back(selected_indexes);
    args.push_back(text);
    args.push_back(key);
    args.push_back(modifiers);

    return app->dispatchPyMethod(m_py_object, "keyPressed", args).toBool();
}

void TreeWidget::clicked(const QModelIndex &index)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int row = index.row();
    int parent_row = -1;
    int parent_id = 0;
    if (index.parent().isValid())
    {
        parent_row = index.parent().row();
        parent_id = (int)(index.parent().internalId());
    }

    app->dispatchPyMethod(m_py_object, "treeItemClicked", QVariantList() << row << parent_row << parent_id);
}

void TreeWidget::doubleClicked(const QModelIndex &index)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int row = index.row();
    int parent_row = -1;
    int parent_id = 0;
    if (index.parent().isValid())
    {
        parent_row = index.parent().row();
        parent_id = (int)(index.parent().internalId());
    }

    app->dispatchPyMethod(m_py_object, "treeItemDoubleClicked", QVariantList() << row << parent_row << parent_id);
}


// -----------------------------------------------------------
// ItemModel
// -----------------------------------------------------------

ItemModel::ItemModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_last_drop_action(Qt::IgnoreAction)
{
}

Qt::DropActions ItemModel::supportedDropActions() const
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    return Qt::DropActions(app->dispatchPyMethod(m_py_object, "supportedDropActions", QVariantList()).toInt());
}

int ItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 1;
}

int ItemModel::rowCount(const QModelIndex &parent) const
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    return app->dispatchPyMethod(m_py_object, "itemCount", QVariantList() << parent.internalId()).toUInt();
}

// All (id=1, parent=0, row=0)
//   Checker (id=11, parent=1, row=0)
//   Green (id=12, parent=1, row=1)
//   Simulator (id=13, parent=1 row=2)
// Some (id=2, parent=0, row=1)
//   Checker (id=21, parent=2, row=0)
//   Green (id=22, parent=2, row=1)

QModelIndex ItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(column)

    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    //qDebug() << "ItemModel::index " << row << "," << parent;
    // ItemModel::index  1 , QModelIndex(-1,-1,0x0,QObject(0x0) )
    // -> QModelIndex(1,0,0x2,ItemModel(0x10f40b7d0

    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int item_id = app->dispatchPyMethod(m_py_object, "itemId", QVariantList() << row << parent.internalId()).toUInt();

    if (row >= 0)
        return createIndex(row, 0, (quint32)item_id);
    return QModelIndex();
}

QModelIndex ItemModel::parent(const QModelIndex &index) const
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariantList result = app->dispatchPyMethod(m_py_object, "itemParent", QVariantList() << index.row() << index.internalId()).toList();

    int row = result[0].toInt();
    int item_id = result[1].toInt();

    if (row >= 0)
        return createIndex(row, 0, (qint32)item_id);
    return QModelIndex();
}

Qt::ItemFlags ItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags default_flags = QAbstractItemModel::flags(index);

    if (index.isValid())
        return default_flags | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
    else
        return default_flags | Qt::ItemIsDropEnabled;
}

QVariant ItemModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QString role_name;
    if (role == Qt::DisplayRole)
        role_name = "display";
    else if (role == Qt::EditRole)
        role_name = "edit";

    //qDebug() << "ItemModel::data " << role_name << ":" << index;

    switch (role)
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            if (index.column() == 0)
            {
                return app->dispatchPyMethod(m_py_object, "itemValue", QVariantList() << role_name << index.row() << index.internalId());
            }
        } break;
    }

    return QVariant(); // QAbstractListModel::data(index, role);
}

bool ItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int row = index.row();
    int parent_row = -1;
    int parent_id = 0;
    if (index.parent().isValid())
    {
        parent_row = index.parent().row();
        parent_id = (int)(index.parent().internalId());
    }

    bool result = app->dispatchPyMethod(m_py_object, "itemSetData", QVariantList() << row << parent_row << parent_id << value).toBool();

    if (result)
        Q_EMIT dataChanged(index, index);

    return result;
}

QStringList ItemModel::mimeTypes() const
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    return app->dispatchPyMethod(m_py_object, "mimeTypesForDrop", QVariantList()).toStringList();
}

QMimeData *ItemModel::mimeData(const QModelIndexList &indexes) const
{
    // simplifying assumption for now
    if (indexes.size() != 1)
        return NULL;

    QModelIndex index = indexes[0];

    int row = index.row();
    int parent_row = -1;
    int parent_id = 0;
    if (index.parent().isValid())
    {
        parent_row = index.parent().row();
        parent_id = (int)(index.parent().internalId());
    }

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariant v_mime_data = app->dispatchPyMethod(m_py_object, "itemMimeData", QVariantList() << row << parent_row << parent_id);

    if (v_mime_data.isNull())
        return NULL;

    QMimeData *mime_data = *(QMimeData **)v_mime_data.constData();

    return mime_data;
}

bool ItemModel::canDropMimeData(const QMimeData *mime_data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (column > 0)
        return false;

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int parent_row = -1;
    int parent_id = 0;
    if (parent.isValid())
    {
        parent_row = parent.row();
        parent_id = (int)(parent.internalId());
    }

    QVariantList args;

    args << QVariant::fromValue((QObject *)mime_data) << (int)action << row << parent_row << parent_id;

    return app->dispatchPyMethod(m_py_object, "canDropMimeData", args).toInt();
}

bool ItemModel::dropMimeData(const QMimeData *mime_data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int parent_row = -1;
    int parent_id = 0;
    if (parent.isValid())
    {
        parent_row = parent.row();
        parent_id = (int)(parent.internalId());
    }

    QVariantList args;

    args << QVariant::fromValue((QObject *)mime_data) << (int)action << row << parent_row << parent_id;

    Qt::DropAction drop_action = (Qt::DropAction)app->dispatchPyMethod(m_py_object, "itemDropMimeData", args).toInt();

    m_last_drop_action = drop_action;

    return drop_action != Qt::IgnoreAction;
}

void ItemModel::beginInsertRowsInParent(int first_row, int last_row, int parent_row, int parent_item_id)
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    QModelIndex parent = parent_row < 0 ? QModelIndex() : createIndex(parent_row, 0, (quint32)parent_item_id);

    beginInsertRows(parent, first_row, last_row);
}

void ItemModel::beginRemoveRowsInParent(int first_row, int last_row, int parent_row, int parent_item_id)
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    QModelIndex parent = parent_row < 0 ? QModelIndex() : createIndex(parent_row, 0, (quint32)parent_item_id);

    beginRemoveRows(parent, first_row, last_row);
}

void ItemModel::endInsertRowsInParent()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    endInsertRows();
}

void ItemModel::endRemoveRowsInParent()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    endRemoveRows();
}

bool ItemModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int parent_row = parent.row();
    int parent_id = (int)(parent.internalId());

    return app->dispatchPyMethod(m_py_object, "removeRows", QVariantList() << row << count << parent_row << parent_id).toBool();
}

void ItemModel::dataChangedInParent(int row, int parent_row, int parent_item_id)
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    QModelIndex parent = parent_row < 0 ? QModelIndex() : createIndex(parent_row, 0, (quint32)parent_item_id);

    Q_EMIT dataChanged(index(row, 0, parent), index(row, 0, parent));
}

QModelIndex ItemModel::indexInParent(int row, int parent_row, int parent_item_id)
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    QModelIndex parent = parent_row < 0 ? QModelIndex() : createIndex(parent_row, 0, (quint32)parent_item_id);

    return index(row, 0, parent);
}

// -----------------------------------------------------------
// PyDrawingContext
// -----------------------------------------------------------

PyDrawingContext::PyDrawingContext(QPainter *painter)
    : m_painter(painter)
{
}

void PyDrawingContext::paintCommands(const QList<CanvasDrawingCommand> &commands)
{
    PaintCommands(*m_painter, commands, &m_image_cache);
}

// -----------------------------------------------------------
// PyStyledItemDelegate
// -----------------------------------------------------------

PyStyledItemDelegate::PyStyledItemDelegate()
{
}

void PyStyledItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem option_copy(option);
    option_copy.text = QString();
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &option_copy, painter, widget);

    painter->save();
    painter->setRenderHints(DEFAULT_RENDER_HINTS | QPainter::SmoothPixmapTransform);

    if (m_py_object.isValid())
    {
        PyDrawingContext *dc = new PyDrawingContext(painter);
        // NOTE: dc is based on painter which is passed to this method. it is only valid during this method call.
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        QVariantMap rect_vm;
        rect_vm["top"] = option.rect.top();
        rect_vm["left"] = option.rect.left();
        rect_vm["width"] = option.rect.width();
        rect_vm["height"] = option.rect.height();
        QVariantMap index_vm;
        int row = index.row();
        int parent_row = -1;
        int parent_id = 0;
        if (index.parent().isValid())
        {
            parent_row = index.parent().row();
            parent_id = (int)(index.parent().internalId());
        }
        index_vm["row"] = row;
        index_vm["parent_row"] = parent_row;
        index_vm["parent_id"] = parent_id;
        QVariantMap paint_info;
        paint_info["rect"] = rect_vm;
        paint_info["index"] = index_vm;
        app->dispatchPyMethod(m_py_object, "paint", QVariantList() << QVariant::fromValue((QObject *)dc) << paint_info);
        delete dc;
    }

    painter->restore();
}

QSize PyStyledItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    int row = index.row();
    int parent_row = -1;
    int parent_id = 0;
    if (index.parent().isValid())
    {
        parent_row = index.parent().row();
        parent_id = (int)(index.parent().internalId());
    }

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariant result = app->dispatchPyMethod(m_py_object, "sizeHint", QVariantList() << row << parent_row << parent_id);

    QVariantList result_list = result.toList();

    return QSize(result_list[0].toInt(), result_list[1].toInt());
}

#include <sstream>
#include <string>
#include <iostream>
#include <stdio.h>
#include <QtWidgets/QLayout>
#include <QtWidgets/QWidget>

#if _MSC_VER
#define snprintf _snprintf
#endif


std::string toString(const QSizePolicy::Policy& policy)
{
    switch (policy) {
        case QSizePolicy::Fixed: return "Fixed";
        case QSizePolicy::Minimum: return "Minimum";
        case QSizePolicy::Maximum: return "Maximum";
        case QSizePolicy::Preferred: return "Preferred";
        case QSizePolicy::MinimumExpanding: return "MinimumExpanding";
        case QSizePolicy::Expanding: return "Expanding";
        case QSizePolicy::Ignored: return "Ignored";
    }
    return "unknown";
}
std::string toString(const QSizePolicy& policy)
{
    return "(" + toString(policy.horizontalPolicy()) + ", " + toString(policy.verticalPolicy()) + ")" ;
}

std::string toString(QLayout::SizeConstraint constraint)
{
    switch (constraint) {
        case QLayout::SetDefaultConstraint: return "SetDefaultConstraint";
        case QLayout::SetNoConstraint: return "SetNoConstraint";
        case QLayout::SetMinimumSize: return "SetMinimumSize";
        case QLayout::SetFixedSize: return "SetFixedSize";
        case QLayout::SetMaximumSize: return "SetMaximumSize";
        case QLayout::SetMinAndMaxSize: return "SetMinAndMaxSize";
    }
    return "unknown";
}
std::string getWidgetInfo(const QWidget& w)
{
    const QRect & geom = w.geometry();
    QSize hint = w.sizeHint();
    char buf[1024];
    snprintf(buf, 1023, "%s %p ('%s'), pos (%d, %d), size (%d x %d), hint (%d x %d) pol: %s %s\n",
             w.metaObject()->className(), (void*)&w, w.objectName().toStdString().c_str(),
             geom.x(), geom.y(), geom.width(), geom.height(),
             hint.width(), hint.height(),
             toString(w.sizePolicy()).c_str(),
             (w.isVisible() ? "" : "**HIDDEN**")
             );
    return buf;
}

std::string getLayoutItemInfo(QLayoutItem* item)
{
    if (dynamic_cast<QWidgetItem*>(item)) {
        QWidgetItem* wi=dynamic_cast<QWidgetItem*>(item);
        if (wi->widget()) {
            return getWidgetInfo(*wi->widget());
        }

    } else if (dynamic_cast<QSpacerItem*>(item)) {
        QSpacerItem* si=dynamic_cast<QSpacerItem*>(item);
        QSize hint=si->sizeHint();
        char buf[1024];
        snprintf(buf, 1023, " SpacerItem hint (%d x %d) policy: %s constraint: ss\n",
                 hint.width(), hint.height(),
                 toString(si->sizePolicy()).c_str()
                 );
        buf[1023] = 0;
        return buf;
    }
    return "";
}

//------------------------------------------------------------------------
void dumpWidgetAndChildren(std::ostream& os, const QWidget* w, int level)
{
    std::string padding("");
    for (int i = 0; i <= level; i++)
        padding+="  ";

    QLayout* layout=w->layout();
    QList<QWidget*> dumpedChildren;
    if (layout && layout->isEmpty()==false) {

        os << padding << "Layout ";
        QMargins margins=layout->contentsMargins();
        os << " margin: (" << margins.left() << "," << margins.top()
        << "," << margins.right() << "," << margins.bottom() << "), constraint: "
        << toString(layout->sizeConstraint());

        if (dynamic_cast<QBoxLayout*>(layout)) {
            QBoxLayout* boxLayout=dynamic_cast<QBoxLayout*>(layout);
            os << " spacing: " <<  boxLayout->spacing();
        }
        os << ":\n";

        int numItems=layout->count();
        for (int i=0; i<numItems; i++) {
            QLayoutItem* layoutItem=layout->itemAt(i);
            std::string itemInfo=getLayoutItemInfo(layoutItem);

            os << padding << " " << itemInfo;

            QWidgetItem* wi=dynamic_cast<QWidgetItem*>(layoutItem);
            if (wi && wi->widget()) {
                dumpWidgetAndChildren(os, wi->widget(), level+1);
                dumpedChildren.push_back(wi->widget());
            }
        }
    }

    // now output any child widgets that weren't dumped as part of the layout
    QList<QWidget *> widgets = w->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    QList<QWidget*> undumpedChildren;
    Q_FOREACH (QWidget* child, widgets) {
        if (dumpedChildren.indexOf(child)==-1) {
            undumpedChildren.push_back(child);
        }
    }

    if (undumpedChildren.empty()==false) {
        os << padding << " non-layout children:\n";
        Q_FOREACH (QWidget* child, undumpedChildren) {
            dumpWidgetAndChildren(os, child, level + 1);
        }
    }
}

//------------------------------------------------------------------------
void dumpWidgetHierarchy(const QWidget* w)
{
    std::ostringstream oss;
    oss << getWidgetInfo(*w);
    dumpWidgetAndChildren(oss, w, 0);
    qDebug() << QString::fromStdString(oss.str());
}

