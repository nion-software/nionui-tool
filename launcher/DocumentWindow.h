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
#if QT_VERSION >= QT_VERSION_CHECK(6,2,0)
#include <QtGui/QAction>
#endif
#include <QtGui/QDrag>
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QtWidgets/QAction>
#endif
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

class DocumentWindow : public QMainWindow
{
    Q_OBJECT

public:
    DocumentWindow(const QString &title, QWidget *parent = 0);

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    void initialize();

protected:
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

    bool m_closed;

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

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    QVariant m_py_object;
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


class PaintImageCacheEntry
{
public:
    PaintImageCacheEntry() { }
    PaintImageCacheEntry(int image_id, bool used, const QImage &image) : image_id(image_id), used(used), image(image) { }
    int image_id;
    bool used;
    QImage image;
};

typedef QMap<int, PaintImageCacheEntry> PaintImageCache;

class LayerCacheEntry
{
public:
    LayerCacheEntry() { }
    LayerCacheEntry(int layer_seed, QSharedPointer<QImage> layer_image, const QRect &layer_rect) : layer_seed(layer_seed), layer_image(layer_image), layer_rect(layer_rect) { }
    int layer_seed;
    QSharedPointer<QImage> layer_image;
    QRect layer_rect;
};

typedef QMap<int, LayerCacheEntry> LayerCache;

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
    PaintImageCache m_image_cache;
};

void PaintCommands(QPainter &painter, const QList<CanvasDrawingCommand> &commands, PaintImageCache *image_cache = NULL, float display_scaling = 0.0);

struct RenderedTimeStamp
{
    RenderedTimeStamp(const QTransform &transform, const QDateTime &dateTime, int section_id) : transform(transform), dateTime(dateTime), section_id(section_id) { }

    QTransform transform;
    QDateTime dateTime;
    int section_id;
};

typedef QList<RenderedTimeStamp> RenderedTimeStamps;

RenderedTimeStamps PaintBinaryCommands(QPainter *painter, const std::vector<quint32> commands, const QMap<QString, QVariant> &imageMap, PaintImageCache *image_cache, LayerCache *layer_cache, float display_scaling = 0.0, int section_id = 0);

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
    void buttonClicked(int button_id);

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

private:
    QVariant m_py_object;
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

class CanvasSection
{
public:
    int m_section_id;
    QMutex m_mutex;
    QList<CanvasDrawingCommand> m_commands;
    std::vector<quint32> m_commands_binary;
    QRect rect;
    QRect image_rect;
    QSharedPointer<QImage> image;
    PaintImageCache m_image_cache;
    LayerCache m_layer_cache;
    QMap<QString, QVariant> m_imageMap;
    RenderedTimeStamps m_rendered_timestamps;
    bool rendering;
    quint64 time;
    QMutex latenciesMutex;
    QQueue<qint64> latencies;
};

struct QRectOptional
{
    QRectOptional(const QRect &rect) : value(rect), has_value(true) { }
    QRectOptional() : has_value(false) { }

    QRect value;
    bool has_value;
};

class PyCanvasRenderTaskSignals : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void renderingReady(const QRect &repaint_rect);
};

class PyCanvasRenderTask : public QRunnable
{
public:
    PyCanvasRenderTask(PyCanvas *canvas);
    ~PyCanvasRenderTask();

    PyCanvasRenderTaskSignals *signals() const { return m_signals; }

    virtual void run() override;

private:
    PyCanvas *m_canvas;
    PyCanvasRenderTaskSignals *m_signals;
};

class PyCanvas : public QWidget
{
    Q_OBJECT
public:
    PyCanvas();
    ~PyCanvas();

    void setPyObject(const QVariant &py_object) { m_py_object = py_object; }

    virtual bool event(QEvent *event) override;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    virtual void enterEvent(QEvent *event) override;
#else
    virtual void enterEvent(QEnterEvent *event) override;
#endif
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
    void setBinaryCommands(const std::vector<quint32> &commands, const QMap<QString, QVariant> &imageMap);

    void setBinarySectionCommands(int section_id, const std::vector<quint32> &commands, const QRect &rect, const QMap<QString, QVariant> &imageMap);
    void removeSection(int section_id);

    void grabMouse0(const QPoint &gp);
    void releaseMouse0();

    QRectOptional renderOne();
    QRectOptional renderSection(QSharedPointer<CanvasSection> section);
    void wakeRenderer();

private Q_SLOTS:
    void renderingFinished();
    void repaintRect(const QRect &rect);

private:
    QVariant m_py_object;
    QMap<QDateTime, QDateTime> m_known_dts;
    QMutex m_commands_mutex;
    QList<CanvasDrawingCommand> m_commands;
    QMap<int, QSharedPointer<CanvasSection> > m_sections;
    std::vector<quint32> m_commands_binary;
    QPoint m_last_pos;
    bool m_pressed;
    unsigned m_grab_mouse_count;
    QPoint m_grab_reference_point;
    QElapsedTimer m_timer;
    QMutex m_rendering_count_mutex;
    int m_rendering_count;
};

QWidget *Widget_makeIntrinsicWidget(const QString &intrinsic_id);
QVariant Widget_getWidgetProperty_(QWidget *widget, const QString &property);
void Widget_setWidgetProperty_(QWidget *view, const QString &property, const QVariant &variant);

#endif  // DOCUMENT_WINDOW_H
