/*
 Copyright (c) 2012-2015 Nion Company.
*/

#include <stdint.h>

#include <QtCore/QtGlobal>
#include <QtCore/QAbstractListModel>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMimeData>
#include <QtCore/QQueue>
#include <QtCore/QStandardPaths>
#include <QtCore/QSettings>
#include <QtCore/QTextCodec>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include <QtGui/QPainter>

#include <QtSvg/QSvgWidget>

#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGestureEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
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

    cleanDocument();
}

void DocumentWindow::initialize()
{
    // TODO: Setup periodic timer
    m_periodic_timer = new QTimer(this);
    connect(m_periodic_timer, SIGNAL(timeout()), this, SLOT(periodic()));
    m_periodic_timer->start(1000.0/50.0);    // 50 times per second

    // reset it here until it is really modified
    cleanDocument();
}

Application *DocumentWindow::application() const
{
    return dynamic_cast<Application *>(QCoreApplication::instance());
}

void DocumentWindow::periodic()
{
    if (isVisible())
        application()->dispatchPyMethod(m_py_object, "periodic", QVariantList());
}

void DocumentWindow::showEvent(QShowEvent *show_event)
{
    QMainWindow::showEvent(show_event);

    // tell python we're closing.
    application()->dispatchPyMethod(m_py_object, "aboutToShow", QVariantList());

    setFocus();
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
    connect(this, SIGNAL(buttonClicked(int)), this, SLOT(buttonClicked(int)));
}

void PyButtonGroup::buttonClicked(int button_id)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "clicked", QVariantList() << button_id);
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
        app->dispatchPyMethod(m_py_object, "value_changed", QVariantList() << value);
    }
}

void PySlider::sliderPressed()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "slider_pressed", QVariantList());
    }
}

void PySlider::sliderReleased()
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "slider_released", QVariantList());
    }
}

void PySlider::sliderMoved(int value)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "slider_moved", QVariantList() << value);
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
        app->dispatchPyMethod(m_py_object, "editing_finished", QVariantList() << text());
    }
}

void PyLineEdit::textEdited(const QString &text)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "text_edited", QVariantList() << text);
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
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        QPoint offset = widget()->mapFrom(viewport(), QPoint(0, 0));
        QRect viewport_rect = viewport()->rect().translated(offset.x(), offset.y());
        app->dispatchPyMethod(m_py_object, "viewportChanged", QVariantList() << viewport_rect.left() << viewport_rect.top() << viewport_rect.width() << viewport_rect.height());
    }
}

void PyScrollArea::scrollBarChanged(int value)
{
    Q_UNUSED(value)

    notifyViewportChanged();
}

void PyScrollArea::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "sizeChanged", QVariantList() << event->size().width() << event->size().height());
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

PyCanvas::PyCanvas()
    : m_pressed(false)
    , m_grab_mouse_count(0)
{
    setMouseTracking(true);
    setAcceptDrops(true);
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

void PaintCommands(QPainter &painter, const QList<CanvasDrawingCommand> &commands, PaintImageCache *image_cache)
{
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

    QMap<int, QGradient> gradients;

    painter.fillRect(painter.viewport(), QBrush(fill_color));

    //qDebug() << "BEGIN";

    QVariantList stack;

    Q_FOREACH(const CanvasDrawingCommand &command, commands)
    {
        QVariantList args = command.arguments;
        QString cmd = command.command;

        //qDebug() << cmd << ": " << args;

        if (cmd == "save")
        {
            QVariantList values;
            values << fill_color << fill_gradient;
            values << line_color << line_width << line_dash << line_cap << line_join;
            values << text_font << text_baseline << text_align;
            stack.push_back(values);
            painter.save();
        }
        else if (cmd == "restore")
        {
            QVariantList values = stack.takeFirst().toList();
            fill_color = values.takeFirst().value<QColor>();
            fill_gradient = values.takeFirst().toInt();
            line_color = values.takeFirst().value<QColor>();
            line_width = values.takeFirst().toFloat();
            line_dash = values.takeFirst().toFloat();
            line_cap = static_cast<Qt::PenCapStyle>(values.takeFirst().toInt());
            line_join = static_cast<Qt::PenJoinStyle>(values.takeFirst().toInt());
            text_font = values.takeFirst().value<QFont>();
            text_baseline = values.takeFirst().toInt();
            text_align = values.takeFirst().toInt();
            painter.restore();
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
            painter.setClipRect(args[0].toFloat(), args[1].toFloat(), args[2].toFloat(), args[3].toFloat(), Qt::IntersectClip);
        }
        else if (cmd == "translate")
        {
            painter.translate(args[0].toFloat(), args[1].toFloat());
        }
        else if (cmd == "scale")
        {
            painter.scale(args[0].toFloat(), args[1].toFloat());
        }
        else if (cmd == "rotate")
        {
            painter.rotate(args[0].toFloat());
        }
        else if (cmd == "moveTo")
        {
            path.moveTo(args[0].toFloat(), args[1].toFloat());
        }
        else if (cmd == "lineTo")
        {
            path.lineTo(args[0].toFloat(), args[1].toFloat());
        }
        else if (cmd == "rect")
        {
            path.addRect(args[0].toFloat(), args[1].toFloat(), args[2].toFloat(), args[3].toFloat());
        }
        else if (cmd == "arc")
        {
            // see http://www.w3.org/TR/2dcontext/#dom-context-2d-arc
            // see https://qt.gitorious.org/qt/qtdeclarative/source/e3eba2902fcf645bf88764f5272e2987e8992cd4:src/quick/items/context2d/qquickcontext2d.cpp#L3801-3815

            float x = args[0].toFloat();
            float y = args[1].toFloat();
            float radius = args[2].toFloat();
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
            QPointF p1(args[0].toFloat(), args[1].toFloat());
            QPointF p2(args[2].toFloat(), args[3].toFloat());
            float radius = args[4].toFloat();

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

            float tangent = radius / tan(acos(cos_phi) / 2);
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
            float sa = acos(orth_p1p0.x() / orth_p1p0_length);
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
            int image_id = args[3].toInt();

            if (image_cache && image_cache->contains(image_id))
            {
                (*image_cache)[image_id].used = true;
                QImage image = (*image_cache)[image_id].image;
                painter.drawImage(QRectF(QPointF(args[4].toFloat(), args[5].toFloat()), QSizeF(args[6].toFloat(), args[7].toFloat())), image);
            }
            else
            {
                Python_ThreadBlock thread_block;

                // Grab the ndarray
                PyObject *ndarray_py = QVariantToPyObject(args[2]);

                if (ndarray_py)
                {
                    QImage image = PythonSupport::instance()->imageFromArray(ndarray_py);

                    if (!image.isNull())
                    {
                        Python_ThreadAllow thread_allow;

                        painter.drawImage(QRectF(QPointF(args[4].toFloat(), args[5].toFloat()), QSizeF(args[6].toFloat(), args[7].toFloat())), image);
                        if (image_cache)
                        {
                            PaintImageCacheEntry cache_entry(image_id, true, image);
                            (*image_cache)[image_id] = cache_entry;
                        }
                    }

                    FreePyObject(ndarray_py);
                }
            }
        }
        else if (cmd == "stroke")
        {
            QPen pen(line_color);
            pen.setWidthF(line_width);
            pen.setJoinStyle(line_join);
            pen.setCapStyle(line_cap);
            if (line_dash > 0.0)
            {
                QVector<qreal> dashes;
                dashes << line_dash << line_dash;
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
            QRegExp re1("^rgba\\(([0-9]+),\\s*([0-9]+),\\s*([0-9]+),\\s*([0-9.]+)\\)$");
            QRegExp re2("^rgb\\(([0-9]+),\\s*([0-9]+),\\s*([0-9]+)\\)$");
            int pos1 = re1.indexIn(color_arg);
            int pos2 = re2.indexIn(color_arg);
            if (pos1 > -1)
                fill_color = QColor(re1.cap(1).toInt(), re1.cap(2).toInt(), re1.cap(3).toInt(), re1.cap(4).toFloat() * 255);
            else if (pos2 > -1)
                fill_color = QColor(re2.cap(1).toInt(), re2.cap(2).toInt(), re2.cap(3).toInt());
            else
                fill_color = QColor(color_arg);
            fill_gradient = -1;
        }
        else if (cmd == "fillStyleGradient")
        {
            fill_gradient = args[0].toInt();
        }
        else if (cmd == "fillText" || cmd == "strokeText")
        {
            QString text = args[0].toString();
            QPointF text_pos(args[1].toFloat(), args[2].toFloat());
            QFontMetrics fm(text_font);
            int text_width = fm.width(text);
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
                pen.setWidth(line_width);
                pen.setJoinStyle(line_join);
                pen.setCapStyle(line_cap);
                painter.strokePath(path, pen);
            }
        }
        else if (cmd == "font")
        {
            QFont font;
            QString family;
            Q_FOREACH(const QString &font_part, args[0].toString().simplified().split(" "))
            {
                if (font_part == "italic")
                    font.setStyle(QFont::StyleItalic);
                if (font_part == "oblique")
                    font.setStyle(QFont::StyleOblique);
                if (font_part == "small-caps")
                    font.setCapitalization(QFont::SmallCaps);
                if (font_part == "bold")
                    font.setWeight(QFont::Bold);
                if (font_part.endsWith("px") && font_part.left(font_part.length() - 2).toInt() > 0)
                    font.setPixelSize(font_part.left(font_part.length() - 2).toInt());
                family = font_part;
            }
            font.setFamily(family);
            text_font = font;
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
            QRegExp re1("^rgba\\(([0-9]+),\\s*([0-9]+),\\s*([0-9]+),\\s*([0-9.]+)\\)$");
            QRegExp re2("^rgb\\(([0-9]+),\\s*([0-9]+),\\s*([0-9]+)\\)$");
            int pos1 = re1.indexIn(color_arg);
            int pos2 = re2.indexIn(color_arg);
            if (pos1 > -1)
                line_color = QColor(re1.cap(1).toInt(), re1.cap(2).toInt(), re1.cap(3).toInt(), re1.cap(4).toFloat() * 255);
            else if (pos2 > -1)
                line_color = QColor(re2.cap(1).toInt(), re2.cap(2).toInt(), re2.cap(3).toInt());
            else
                line_color = QColor(color_arg);
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
            gradients[args[0].toInt()] = QLinearGradient(args[3].toFloat(), args[4].toFloat(), args[3].toFloat() + args[5].toFloat(), args[4].toFloat() + args[6].toFloat());
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

void PyCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter;

    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QList<CanvasDrawingCommand> commands;

    {
        QMutexLocker locker(&m_commands_mutex);
        commands = m_commands;
    }

    //QDateTime start = QDateTime::currentDateTime();

    PaintCommands(painter, commands, &m_image_cache);
#if 0
    static int frameCount = 0;
    static float dt = 0.0;
    static float fps = 0.0;
    static float updateRate = 1.0;  // 4 updates per sec.
    static QTime time;

    frameCount++;
    dt += time.elapsed() / 1000.0;
    if (dt > 1.0/updateRate)
    {
        fps = frameCount / dt ;
        qDebug() << "pp fps " << fps;
        frameCount = 0;
        dt -= 1.0/updateRate;
    }
    time.start();
#endif

    //qDebug() << "Paint " << QDateTime::currentDateTime() << " elapsed " << start.msecsTo(QDateTime::currentDateTime())/1000.0;

    painter.end();
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
                if (app->dispatchPyMethod(m_py_object, "panGesture", QVariantList() << pan_gesture->delta().x() << pan_gesture->delta().y()).toBool())
                    return true;
            }
            QPinchGesture *pinch_gesture = static_cast<QPinchGesture *>(gesture_event->gesture(Qt::PinchGesture));
            if (pinch_gesture)
            {
                qDebug() << "pinch";
            }
        } break;
        default: break;
    }
    return QWidget::event(event);
}

void PyCanvas::enterEvent(QEvent *event)
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
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mousePressed", QVariantList() << event->x() << event->y() << (int)event->modifiers());
        m_last_pos = event->pos();
        m_pressed = true;
    }
}

void PyCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_py_object.isValid() && event->button() == Qt::LeftButton)
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mouseReleased", QVariantList() << event->x() << event->y() << (int)event->modifiers());
        m_pressed = false;

        if ((event->pos() - m_last_pos).manhattanLength() < 6)
        {
            app->dispatchPyMethod(m_py_object, "mouseClicked", QVariantList() << event->x() << event->y() << (int)event->modifiers());
        }
    }
}

void PyCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_py_object.isValid() && event->button() == Qt::LeftButton)
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "mouseDoubleClicked", QVariantList() << event->x() << event->y() << (int)event->modifiers());
    }
}

void PyCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

        if (m_grab_mouse_count > 0)
        {
            QPoint delta = event->pos() - m_grab_reference_point;

            app->dispatchPyMethod(m_py_object, "grabbedMousePositionChanged", QVariantList() << delta.x() << delta.y() << (int)event->modifiers());

            QCursor::setPos(mapToGlobal(m_grab_reference_point));
            QApplication::changeOverrideCursor(Qt::BlankCursor);
        }

        app->dispatchPyMethod(m_py_object, "mousePositionChanged", QVariantList() << event->x() << event->y() << (int)event->modifiers());

        // handle case of not getting mouse released event after drag.
        if (m_pressed && !(event->buttons() & Qt::LeftButton))
        {
            app->dispatchPyMethod(m_py_object, "mouseReleased", QVariantList() << event->x() << event->y() << (int)event->modifiers());
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
        bool is_horizontal = wheel_event->orientation() == Qt::Horizontal;
        QPoint delta = wheel_event->pixelDelta().isNull() ? wheel_event->angleDelta() : wheel_event->pixelDelta();
        app->dispatchPyMethod(m_py_object, "wheelChanged", QVariantList() << wheel_event->x() << wheel_event->y() << delta.x() << delta.y() << (bool)is_horizontal);
    }
}

void PyCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "sizeChanged", QVariantList() << event->size().width() << event->size().height());
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

    args.push_back(event->pos().x());
    args.push_back(event->pos().y());
    args.push_back(event->globalPos().x());
    args.push_back(event->globalPos().y());

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

void PyCanvas::setCommands(const QList<CanvasDrawingCommand> &commands)
{
    {
        QMutexLocker locker(&m_commands_mutex);
        m_commands = commands;
    }

    QTimer::singleShot(0, this, SLOT(update()));
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
        QString action = app->dispatchPyMethod(m_py_object, "dragMoveEvent", QVariantList() << QVariant::fromValue((QObject *)event->mimeData()) << event->pos().x() << event->pos().y()).toString();
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
        QString action = app->dispatchPyMethod(m_py_object, "dropEvent", QVariantList() << QVariant::fromValue((QObject *)event->mimeData()) << event->pos().x() << event->pos().y()).toString();
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

QWidget *Widget_makeIntrinsicWidget(const QString &intrinsic_id)
{
    if (intrinsic_id == "row")
    {
        QWidget *row = new QWidget();
        QHBoxLayout *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        row_layout->setSpacing(0);
        return row;
    }
    else if (intrinsic_id == "column")
    {
        QWidget *column = new QWidget();
        QVBoxLayout *column_layout = new QVBoxLayout(column);
        column_layout->setContentsMargins(0, 0, 0, 0);
        column_layout->setSpacing(0);
        return column;
    }
    else if (intrinsic_id == "tab")
    {
        PyTabWidget *group = new PyTabWidget();
        group->setTabsClosable(false);
        group->setMovable(false);
        QFile stylesheet_file(":/app/stylesheet.qss");
        if (stylesheet_file.open(QIODevice::ReadOnly))
        {
            QString stylesheet = stylesheet_file.readAll();
            group->setStyleSheet(stylesheet);
            stylesheet_file.close();
        }
        return group;
    }
    else if (intrinsic_id == "stack")
    {
        QStackedWidget *stack = new QStackedWidget();
        QFile stylesheet_file(":/app/stylesheet.qss");
        if (stylesheet_file.open(QIODevice::ReadOnly))
        {
            QString stylesheet = stylesheet_file.readAll();
            stack->setStyleSheet(stylesheet);
            stylesheet_file.close();
        }
        return stack;
    }
    else if (intrinsic_id == "scrollarea")
    {
        PyScrollArea *scroll_area = new PyScrollArea();
        // Set up the system wide stylesheet
        QFile stylesheet_file(":/app/stylesheet.qss");
        if (stylesheet_file.open(QIODevice::ReadOnly))
        {
            scroll_area->setStyleSheet(stylesheet_file.readAll());
            stylesheet_file.close();
        }
        return scroll_area;
    }
    else if (intrinsic_id == "splitter")
    {
        QSplitter *splitter = new QSplitter();
        splitter->setOrientation(Qt::Vertical);
        QFile stylesheet_file(":/app/stylesheet.qss");
        if (stylesheet_file.open(QIODevice::ReadOnly))
        {
            QString stylesheet = stylesheet_file.readAll();
            splitter->setStyleSheet(stylesheet);
            stylesheet_file.close();
        }
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
        QFile stylesheet_file(":/app/stylesheet.qss");
        if (stylesheet_file.open(QIODevice::ReadOnly))
        {
            scroll_area->setStyleSheet(stylesheet_file.readAll());
            stylesheet_file.close();
        }

        QWidget *content_view = new QWidget();
        content_view->setContentsMargins(0, 0, 0, 0);
        content_view->setStyleSheet("border: none; background-color: transparent");
        QVBoxLayout *content_view_layout = new QVBoxLayout(content_view);
        content_view_layout->setContentsMargins(0, 0, 0, 0);
        content_view_layout->setSpacing(0);
        content_view_layout->addWidget(scroll_area);

        return content_view;
    }
    else if (intrinsic_id == "pylist")
    {
        ListWidget *py_list_widget = new ListWidget();
        py_list_widget->setStyleSheet("QListView { border: none; background-color: transparent; }");

        QScrollArea *scroll_area = new QScrollArea();
        scroll_area->setWidgetResizable(true);
        scroll_area->setWidget(py_list_widget);
        scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        // Set up the system wide stylesheet
        QFile stylesheet_file(":/app/stylesheet.qss");
        if (stylesheet_file.open(QIODevice::ReadOnly))
        {
            scroll_area->setStyleSheet(stylesheet_file.readAll());
            stylesheet_file.close();
        }

        QWidget *content_view = new QWidget();
        content_view->setContentsMargins(0, 0, 0, 0);
        content_view->setStyleSheet("border: none");
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

void Widget_setWidgetProperty_(QWidget *widget, const QString &property, const QVariant &variant)
{
    if (property == "margin")
    {
        int margin = variant.toInt();
        widget->setContentsMargins(margin, margin, margin, margin);
    }
    else if (property == "margin-top")
    {
        int value = variant.toInt();
        QMargins margin = widget->contentsMargins();
        margin.setTop(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "margin-left")
    {
        int value = variant.toInt();
        QMargins margin = widget->contentsMargins();
        margin.setLeft(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "margin-bottom")
    {
        int value = variant.toInt();
        QMargins margin = widget->contentsMargins();
        margin.setBottom(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "margin-right")
    {
        int value = variant.toInt();
        QMargins margin = widget->contentsMargins();
        margin.setRight(value);
        widget->setContentsMargins(margin);
    }
    else if (property == "min-width")
    {
        widget->setMinimumWidth(variant.toInt());
        QSizePolicy size_policy = widget->sizePolicy();
        size_policy.setHorizontalPolicy(QSizePolicy::Expanding);
        widget->setSizePolicy(size_policy);
    }
    else if (property == "min-height")
    {
        widget->setMinimumHeight(variant.toInt());
        QSizePolicy size_policy = widget->sizePolicy();
        size_policy.setVerticalPolicy(QSizePolicy::Expanding);
        widget->setSizePolicy(size_policy);
    }
    else if (property == "width")
    {
        widget->setMinimumWidth(variant.toInt());
        widget->setMaximumWidth(variant.toInt());
    }
    else if (property == "height")
    {
        widget->setMinimumHeight(variant.toInt());
        widget->setMaximumHeight(variant.toInt());
    }
    else if (property == "spacing")
    {
        QBoxLayout *layout = dynamic_cast<QBoxLayout *>(widget->layout());
        if (layout)
            layout->setSpacing(variant.toInt());
    }
    else if (property == "font-size")
    {
        QFont font = widget->font();
        font.setPointSize(variant.toInt());
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
    Qt::DropAction action = exec();
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

ItemModel::ItemModel(const QStringList &role_names, QObject *parent)
    : QAbstractItemModel(parent)
    , m_last_drop_action(Qt::IgnoreAction)
{
    int role = Qt::UserRole + 1;
    Q_FOREACH(const QString &role_name, role_names)
    {
        m_role_names.insert(role, role_name.toUtf8());
        ++role;
    }
}

QHash<int, QByteArray> ItemModel::roleNames() const
{
    return m_role_names;
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
    else if (m_role_names.contains(role))
        role_name = m_role_names.value(role);
    else
        role_name = "unknown";

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

bool ItemModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(data)
    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)

    // see https://bugreports.qt-project.org/browse/QTBUG-32362
    return false;
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
// ListWidget
// -----------------------------------------------------------

ListWidget::ListWidget()
{
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragEnabled(true);

    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(clicked(QModelIndex)));
    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleClicked(QModelIndex)));
}

void ListWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusIn", QVariantList());
    }

    QListView::focusInEvent(event);
}

void ListWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    if (m_py_object.isValid())
    {
        Application *app = dynamic_cast<Application *>(QCoreApplication::instance());
        app->dispatchPyMethod(m_py_object, "focusOut", QVariantList());
    }

    QListView::focusOutEvent(event);
}

void ListWidget::setModelAndConnect(ListModel *py_list_model)
{
    setModel(py_list_model);

    connect(py_list_model, SIGNAL(modelAboutToBeReset()), this, SLOT(modelAboutToBeReset()));
    connect(py_list_model, SIGNAL(modelReset()), this, SLOT(modelReset()));
}

void ListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress && handleKey(event->text(), event->key(), event->modifiers()))
        return;

    QListView::keyPressEvent(event);
}

void ListWidget::dropEvent(QDropEvent *event)
{
    QListView::dropEvent(event);
    if (event->isAccepted())
    {
        ListModel *py_list_model = dynamic_cast<ListModel *>(model());
        event->setDropAction(py_list_model->lastDropAction());
    }
}

void ListWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QListView::currentChanged(current, previous);

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int row = current.row();

    app->dispatchPyMethod(m_py_object, "listItemChanged", QVariantList() << row);
}

void ListWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    // note the parameters passed represent the CHANGES not the new and old selection

    QListView::selectionChanged(selected, deselected);

    QVariantList selected_indexes;

    Q_FOREACH(const QModelIndex &index, selectedIndexes())
    {
        selected_indexes.push_back(index.row());
    }

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariantList args;

    args.push_back(selected_indexes);

    app->dispatchPyMethod(m_py_object, "listSelectionChanged", args);
}

void ListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariantList args;

    args.push_back(event->pos().x());
    args.push_back(event->pos().y());
    args.push_back(event->globalPos().x());
    args.push_back(event->globalPos().y());

    app->dispatchPyMethod(m_py_object, "contextMenuEvent", args);
}

void ListWidget::modelAboutToBeReset()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    m_saved_index = currentIndex().row();
}

void ListWidget::modelReset()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    setCurrentIndex(model()->index(m_saved_index, 0));
}

bool ListWidget::handleKey(const QString &text, int key, int modifiers)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariantList selected_indexes;

    Q_FOREACH(const QModelIndex &index, selectedIndexes())
    {
        selected_indexes.push_back(index.row());
    }

    if (selected_indexes.size() == 1)
    {
        if (app->dispatchPyMethod(m_py_object, "listItemKeyPressed", QVariantList() << selected_indexes[0] << text << key << modifiers).toBool())
            return true;
    }

    QVariantList args;

    args.push_back(selected_indexes);
    args.push_back(text);
    args.push_back(key);
    args.push_back(modifiers);

    return app->dispatchPyMethod(m_py_object, "keyPressed", args).toBool();
}

void ListWidget::clicked(const QModelIndex &index)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int row = index.row();

    app->dispatchPyMethod(m_py_object, "listItemClicked", QVariantList() << row);
}

void ListWidget::doubleClicked(const QModelIndex &index)
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    int row = index.row();

    app->dispatchPyMethod(m_py_object, "listItemDoubleClicked", QVariantList() << row);
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
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

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


// -----------------------------------------------------------
// ListModel
// -----------------------------------------------------------

ListModel::ListModel(const QStringList &role_names, QObject *parent)
    : QAbstractListModel(parent)
    , m_last_drop_action(Qt::IgnoreAction)
    , m_row_count(0)
{
    int role = Qt::UserRole + 1;
    Q_FOREACH(const QString &role_name, role_names)
    {
        m_role_names.insert(role, role_name.toUtf8());
        ++role;
    }
}

ListModel::~ListModel()
{
}

QHash<int, QByteArray> ListModel::roleNames() const
{
    return m_role_names;
}

Qt::DropActions ListModel::supportedDropActions() const
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    return Qt::DropActions(app->dispatchPyMethod(m_py_object, "supportedDropActions", QVariantList()).toInt());
}

int ListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 1;
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_row_count;
}

Qt::ItemFlags ListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags default_flags = QAbstractListModel::flags(index);

    if (index.isValid())
    {
        return default_flags | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
    }
    else
        return default_flags | Qt::ItemIsDropEnabled;
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QString role_name;
    if (role == Qt::DisplayRole)
        role_name = "display";
    else if (role == Qt::EditRole)
        role_name = "edit";
    else if (m_role_names.contains(role))
        role_name = m_role_names.value(role);
    else
        role_name = "unknown";

    if (role == Qt::DisplayRole)
    {
        return app->dispatchPyMethod(m_py_object, "itemValue", QVariantList() << role_name << index.row());
    }

    return QVariant();
}

QStringList ListModel::mimeTypes() const
{
    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    return app->dispatchPyMethod(m_py_object, "mimeTypesForDrop", QVariantList()).toStringList();
}

QMimeData *ListModel::mimeData(const QModelIndexList &indexes) const
{
    // simplifying assumption for now
    if (indexes.size() != 1)
        return NULL;

    QModelIndex index = indexes[0];

    int row = index.row();

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    QVariant v_mime_data = app->dispatchPyMethod(m_py_object, "itemMimeData", QVariantList() << row);

    if (v_mime_data.isNull())
        return NULL;

    QMimeData *mime_data = *(QMimeData **)v_mime_data.constData();

    return mime_data;
}

bool ListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(data)
    Q_UNUSED(action)
    Q_UNUSED(parent)
    Q_UNUSED(column)
    Q_UNUSED(row)

    // see https://bugreports.qt-project.org/browse/QTBUG-32362
    return false;
}

bool ListModel::dropMimeData(const QMimeData *mime_data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    // parent row allows clients to distinguish between dropping on or between rows
    int parent_row = -1;
    if (parent.isValid())
        parent_row = parent.row();

    QVariantList args;

    args << QVariant::fromValue((QObject *)mime_data) << (int)action << row << parent_row;

    Qt::DropAction drop_action = (Qt::DropAction)app->dispatchPyMethod(m_py_object, "itemDropMimeData", args).toInt();

    m_last_drop_action = drop_action;

    return drop_action != Qt::IgnoreAction;
}

void ListModel::beginInsertRowsInList(int first_row, int last_row)
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    beginInsertRows(QModelIndex(), first_row, last_row);
    m_row_count += last_row - first_row + 1;
}

void ListModel::beginRemoveRowsInList(int first_row, int last_row)
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    beginRemoveRows(QModelIndex(), first_row, last_row);
    m_row_count -= last_row - first_row + 1;
}

void ListModel::endInsertRowsInList()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    endInsertRows();
}

void ListModel::endRemoveRowsInList()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    endRemoveRows();
}

bool ListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    Application *app = dynamic_cast<Application *>(QCoreApplication::instance());

    return app->dispatchPyMethod(m_py_object, "removeRows", QVariantList() << row << count).toBool();
}

void ListModel::dataChangedInList()
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());
    int rows = rowCount(QModelIndex());
    Q_EMIT dataChanged(index(0,0), index(rows-1, 0));
}
