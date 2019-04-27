/*
 Copyright (c) 2012-2015 Nion Company.
*/

#include <QtGui/QImage>

#ifndef PYTHON_SUPPORT_H
#define PYTHON_SUPPORT_H

#pragma push_macro("_DEBUG")
#undef _DEBUG
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#pragma pop_macro("_DEBUG")

#define PyInt_Check PyLong_Check
#define PyInt_FromLong CALL_PY(PyLong_FromLong)
#define PyInt_AsLong CALL_PY(PyLong_AsLong)
#define PyString_FromString CALL_PY(PyUnicode_FromString)
#define PyString_Check PyUnicode_Check
#define PyCodeObject PyObject

// Use this when calling back to Python code to grab the GIL and release it when the
// Python code returns.
struct Python_ThreadBlockState;
class Python_ThreadBlock
{
public:
    Python_ThreadBlock() : m_state(NULL) { grab(); }
    ~Python_ThreadBlock() { release(); }
    void release();
    void grab();
private:
    Python_ThreadBlockState *m_state;
};

// Use this when being called from Python to save the current thread, release the GIL,
// and then grab the GIL and restore the current thread when returning to Python.
struct Python_ThreadAllowState;
class Python_ThreadAllow
{
public:
    Python_ThreadAllow() : m_state(NULL) { grab(); }
    ~Python_ThreadAllow() { release(); }
    void release();
    void grab();
private:
    Python_ThreadAllowState *m_state;
};

PyObject *WrapQObject(QObject *ptr);

QVariant PyObjectToQVariant(PyObject *py_object);
PyObject *QVariantToPyObject(const QVariant &value);
PyObject* QStringToPyObject(const QString& str);
void FreePyObject(PyObject *py_object);

typedef PyObject *CreateAndAddModuleFn();

class PythonSupport
{
public:
    typedef int (*PyArg_ParseTupleFn)(PyObject *, const char *, ...);
    typedef PyObject* (*Py_BuildValueFn)(const char *, ...);

    static PythonSupport *instance();

    // only called once
    static void initInstance(const QString &python_home);
    static void deinitInstance();

    static QString ensurePython(const QString &python_home);

    static void checkTarget(const QString &python_path);

    void initialize(const QString &python_home);
    void deinitialize();
    void addResourcePath(const QString &resources_path);
	void addPyObjectToModuleFromQVariant(PyObject* module, const QString &identifier, const QVariant& object);
	void addPyObjectToModule(PyObject* module, const QString &identifier, PyObject *object);
    QImage imageFromRGBA(PyObject *ndarray_py);
    QImage imageFromArray(PyObject *ndarray_py, float display_limit_low, float display_limit_high, PyObject *lookup_table);
    QImage scaledImageFromArray(PyObject *ndarray_py, const QSizeF &destination_size, float context_scaling, float display_limit_low, float display_limit_high, PyObject *lookup_table);
    PyObject *arrayFromImage(const QImage &image);
    void bufferRelease(Py_buffer *buffer);
    QVariant invokePyMethod(const QVariant &object, const QString &method, const QVariantList &args);
    bool setAttribute(const QVariant &object, const QString &attribute, const QVariant &value);
    QVariant getAttribute(const QVariant &object, const QString &attribute);
    void setErrorString(const QString &error_string);
    PyObject *getPyListFromStrings(const QStringList &strings);
    PyArg_ParseTupleFn parse();
    Py_BuildValueFn build();
    PyObject *getNoneReturnValue();
    bool isNone(PyObject *obj);
    PyObject *createAndAddModule(PyModuleDef *moduledef);
    void prepareModuleException(const char *name);
    void initializeModule(const char *name, CreateAndAddModuleFn fn);
    void printAndClearErrors();
    PyObject *import(const char *name);
    QObject *UnwrapQObject(PyObject *py_object);
	static const char* qobject_capsule_name;
private:
    PythonSupport(const QString &python_home); // ctor hidden
    PythonSupport(PythonSupport const&); // copy ctor hidden
    PythonSupport& operator=(PythonSupport const&); // assign op. hidden
    ~PythonSupport(); // dtor hidden

    // dynamics
    PyArg_ParseTupleFn dynamic_PyArg_ParseTuple;
    Py_BuildValueFn dynamic_Py_BuildValue;

    // exceptions
    PyObject *module_exception;
};

template <typename T> inline T *Unwrap(PyObject *py_object) { return dynamic_cast<T *>(PythonSupport::instance()->UnwrapQObject(py_object)); }

#endif // PYTHON_SUPPORT_H
