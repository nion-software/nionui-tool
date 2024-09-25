/*
 Copyright (c) 2012-2015 Nion Company.
*/

#ifndef DOCUMENT_WINDOW_H
#define DOCUMENT_WINDOW_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QRunnable>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtGui/QAction>
#include <QtGui/QDrag>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeView>

class QCheckBox;
class QFileDialog;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QQuickView;
class QVBoxLayout;

class Application;
class ItemModel;
class ListModel;

class PyCanvas;

class DocumentWindow : public QMainWindow
{
    Q_OBJECT

public:
    DocumentWindow(const QString &title, QWidget *parent = 0);

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    void initialize();

    void requestRepaint(PyCanvas *canvas);
    void cancelRepaintRequest(PyCanvas *canvas);

public Q_SLOTS:
    void screenChanged(QScreen *screen);
    void logicalDotsPerInchChanged(qreal dpi);
    void physicalDotsPerInchChanged(qreal dpi);
    void colorSchemeChanged(Qt::ColorScheme colorScheme);

protected:
    virtual void hideEvent(QHideEvent *hide_event) override;
    virtual void showEvent(QShowEvent *show_event) override;
    virtual void changeEvent(QEvent * event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void moveEvent(QMoveEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

private:
    // check to save document
    virtual void closeEvent(QCloseEvent *close_event) override;
    virtual void timerEvent(QTimerEvent *event) override;

    // mark the document as clean
    void cleanDocument();

    QVariant m_py_object;

    int m_periodic_timer;

    QMutex m_repaint_mutex;

    bool m_closed;

    QScreen *m_screen;

    QSet<PyCanvas *> m_repaint_requests;

    Application *application() const;

    friend class Application;
};


class DockWidget : public QDockWidget
{
    Q_OBJECT

public:
    DockWidget(const QString &title, QWidget *parent = 0);

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

public Q_SLOTS:
    void screenChanged(QScreen *screen);
    void logicalDotsPerInchChanged(qreal dpi);
    void physicalDotsPerInchChanged(qreal dpi);

protected:
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void hideEvent(QHideEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;

private:
    QVariant m_py_object;
    QScreen *m_screen;
};


class PyAction : public QAction
{
    Q_OBJECT

public:
    PyAction(QObject *parent);

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

public Q_SLOTS:
    void triggered();

private:
    QVariant m_py_object;
};


class PyMenu : public QMenu
{
    Q_OBJECT

public:
    PyMenu();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

public Q_SLOTS:
    void aboutToShow();
    void aboutToHide();

private:
    QVariant m_py_object;
};


class Drag : public QDrag
{
    Q_OBJECT

public:
    Drag(QWidget *widget);

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

public Q_SLOTS:
    void execute();

private:
    QVariant m_py_object;
};


class TreeWidget : public QTreeView
{
    Q_OBJECT

public:
    TreeWidget();

    void setModelAndConnect(ItemModel *py_item_model);

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    // Override
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;

    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

public Q_SLOTS:
    void modelAboutToBeReset();
    void modelReset();
    void clicked(const QModelIndex &index);
    void doubleClicked(const QModelIndex &index);

protected:
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

private:
    bool handleKey(const QString &text, int key, int modifiers);

    QVariant m_py_object;
    int m_saved_index;
};


class ItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ItemModel(QObject *parent = 0);

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    Qt::DropAction lastDropAction() const { return m_last_drop_action; }

    // public methods
    void beginInsertRowsInParent(int first_row, int last_row, int parent_row, int parent_item_id);
    void beginRemoveRowsInParent(int first_row, int last_row, int parent_row, int parent_item_id);
    void endInsertRowsInParent();
    void endRemoveRowsInParent();
    void dataChangedInParent(int row, int parent_row, int parent_item_id);
    QModelIndex indexInParent(int index, int parent_row, int parent_item_id);

    // from QAbstractListModel
    virtual Qt::DropActions supportedDropActions() const override;
    virtual int columnCount (const QModelIndex & parent) const override;
    virtual int rowCount (const QModelIndex & parent) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual bool removeRows(int row, int count, const QModelIndex &parent) override;
    virtual QStringList mimeTypes() const override;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &index) const override;

private:
    QVariant m_py_object;
    Qt::DropAction m_last_drop_action;
};

struct CanvasDrawingCommand
{
    QString command;
    QList<QVariant> arguments;
};

class PyDrawingContext : public QObject
{
    Q_OBJECT
public:
    PyDrawingContext(QPainter *painter);
    void paintCommands(const QList<CanvasDrawingCommand> &commands);
private:
    QPainter *m_painter;
};

void PaintCommands(QPainter &painter, const QList<CanvasDrawingCommand> &commands, float display_scaling = 0.0);

struct RenderedTimeStamp
{
    RenderedTimeStamp(const QTransform &transform, const int64_t &timestamp_ns, int section_id, int64_t elapsed_ns = 0, const QString text = QString()) : transform(transform), timestamp_ns(timestamp_ns), section_id(section_id), elapsed_ns(elapsed_ns), text(text) { }

    QTransform transform;
    int64_t timestamp_ns;
    int64_t elapsed_ns;
    QString text;
    int section_id;
};

typedef QList<RenderedTimeStamp> RenderedTimeStamps;

typedef std::shared_ptr<std::vector<quint32>> CommandsSharedPtr;

RenderedTimeStamps PaintBinaryCommands(QPainter *painter, const CommandsSharedPtr &commands, const QMap<QString, QVariant> &imageMap, const RenderedTimeStamps &lastRenderedTimestamps, float display_scaling = 0.0, int section_id = 0, float devicePixelRatio = 1.0);

class PyStyledItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    PyStyledItemDelegate();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private Q_SLOTS:
    // None

private:
    QVariant m_py_object;
};

class PyPushButton : public QPushButton
{
    Q_OBJECT
public:
    PyPushButton();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

private Q_SLOTS:
    void clicked();

private:
    QVariant m_py_object;
};

class PyRadioButton : public QRadioButton
{
    Q_OBJECT
public:
    PyRadioButton();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

private Q_SLOTS:
    void clicked();

private:
    QVariant m_py_object;
};

class PyButtonGroup : public QButtonGroup
{
    Q_OBJECT
public:
    PyButtonGroup();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

private Q_SLOTS:
    void buttonClicked(QAbstractButton *button);

private:
    QVariant m_py_object;
};

class PyTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    PyTextEdit();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void insertFromMimeData(const QMimeData *source) override;

private Q_SLOTS:
    void cursorPositionChanged();
    void selectionChanged();
    void textChanged();

private:
    QVariant m_py_object;
};

class PyTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    PyTextBrowser();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual QVariant loadResource(int type, const QUrl &name) override;

private Q_SLOTS:
    void anchorClicked(const QUrl &link);

private:
    QVariant m_py_object;
};

class PyCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    PyCheckBox();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

private Q_SLOTS:
    void stateChanged(int state);

private:
    QVariant m_py_object;
};

class PyComboBox : public QComboBox
{
    Q_OBJECT
public:
    PyComboBox();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

private Q_SLOTS:
    void currentTextChanged(const QString &currentText);

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    QVariant m_py_object;
    bool isExpanded();
};

class PySlider : public QSlider
{
    Q_OBJECT
public:
    PySlider();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

private Q_SLOTS:
    void valueChanged(int value);
    void sliderPressed();
    void sliderReleased();
    void sliderMoved(int);

private:
    QVariant m_py_object;
};

class PyLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    PyLineEdit();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual void keyPressEvent(QKeyEvent *event) override;

private Q_SLOTS:
    void editingFinished();
    void textEdited(const QString &text);

private:
    QVariant m_py_object;
};

class PyScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    PyScrollArea();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual void resizeEvent(QResizeEvent *event) override;
    virtual bool eventFilter(QObject *obj, QEvent *event) override;

    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

public Q_SLOTS:
    void scrollBarChanged(int value);

private:
    QVariant m_py_object;

    void notifyViewportChanged();
};

class PyTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    PyTabWidget();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

public Q_SLOTS:
    void currentChanged(int index);

private:
    QVariant m_py_object;
};

class Overlay : public QWidget
{
    Q_OBJECT
public:
    Overlay(QWidget *parent, QWidget *child);
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
private:
    QWidget *m_child;
};

class PyCanvas;
class PyCanvasRenderTask;

class DrawingCommands
{
public:
    DrawingCommands(const CommandsSharedPtr &commands, const QRect &rect, const QMap<QString, QVariant> &image_map)
    : m_commands(commands), m_rect(rect), m_image_map(image_map) { }

    const CommandsSharedPtr commands() const { return m_commands; }
    const QMap<QString, QVariant> &imageMap() const { return m_image_map; }
    const QRect &rect() const { return m_rect; }
private:
    CommandsSharedPtr m_commands;
    QMap<QString, QVariant> m_image_map;
    QRect m_rect;
};

typedef std::shared_ptr<DrawingCommands> DrawingCommandsSharedPtr;

class CanvasSection
{
public:
    int m_section_id;
    DrawingCommandsSharedPtr m_pending_drawing_commands;
    float m_device_pixel_ratio;
    QRect image_rect;
    QSharedPointer<QImage> image;
    RenderedTimeStamps m_rendered_timestamps;
    PyCanvasRenderTask *m_render_task;
    QQueue<int64_t> latencies_ns;
    QQueue<int64_t> timestamps_ns;
    bool record_latency;

    CanvasSection(int section_id, float device_pixel_ratio);
};

typedef std::shared_ptr<CanvasSection> CanvasSectionSharedPtr;

struct QRectOptional
{
    QRectOptional(const QRect &rect) : value(rect), has_value(true) { }
    QRectOptional() : has_value(false) { }

    QRect value;
    bool has_value;
};

struct RenderResult
{
    CanvasSectionSharedPtr section;
    RenderedTimeStamps rendered_timestamps;
    QSharedPointer<QImage> image;
    QRect image_rect;
    bool record_latency;

    RenderResult(const CanvasSectionSharedPtr &section) : section(section), record_latency(false) { }
};

/*
 A task to render a canvas section.

 The rendered timestamps are passed in as const. They are not updated on the section directly and instead a
 new copy is put into the RenderResult and the section is updated at paint time.
 */
class PyCanvasRenderTask : public QRunnable
{
public:
    PyCanvasRenderTask(PyCanvas *canvas, const CanvasSectionSharedPtr &section, const DrawingCommandsSharedPtr &drawing_commands, float devicePixelRatio, const RenderedTimeStamps &rendered_timestamps);

    virtual void run() override;

    const CanvasSectionSharedPtr section() const { return m_section; }

private:
    PyCanvas *m_canvas;
    const CanvasSectionSharedPtr m_section;
    const DrawingCommandsSharedPtr m_drawing_commands;
    float m_device_pixel_ratio;
    const RenderedTimeStamps m_rendered_timestamps;
};

class PyCanvas : public QWidget
{
    Q_OBJECT
public:
    PyCanvas();
    ~PyCanvas();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual bool event(QEvent *event) override;

    virtual void enterEvent(QEnterEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

    virtual void contextMenuEvent(QContextMenuEvent *e) override;

    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;

    void setCommands(const QList<CanvasDrawingCommand> &commands);
    void setBinarySectionCommands(int section_id, const DrawingCommandsSharedPtr &drawing_commands);
    void removeSection(int section_id);

    void grabMouse0(const QPoint &gp);
    void releaseMouse0();

    QElapsedTimer total_timer;

    void continuePaintingSection(const RenderResult &render_result);

private:
    QVariant m_py_object;
    QMutex m_sections_mutex;
    QMap<int, CanvasSectionSharedPtr> m_sections;
    QPoint m_last_pos;
    bool m_pressed;
    unsigned m_grab_mouse_count;
    QPoint m_grab_reference_point;
    DocumentWindow *m_document_window;
};

QWidget *Widget_makeIntrinsicWidget(const QString &intrinsic_id);
QVariant Widget_getWidgetProperty_(QWidget *widget, const QString &property);
void Widget_setWidgetProperty_(QWidget *view, const QString &property, const QVariant &variant);

#endif  // DOCUMENT_WINDOW_H
