/*
 Copyright (c) 2012-2015 Nion Company.
*/

#ifndef PYTHON_SUPPORT_H
#define PYTHON_SUPPORT_H

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#pragma push_macro("_DEBUG")
#undef _DEBUG
#define PY_SSIZE_T_CLEAN
#define MS_NO_COREDLL 1
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

class PyObjectPtr
{
public:
    PyObjectPtr(PyObject *py_object = nullptr, bool borrowed = false) : py_object(py_object)
    {
        if (borrowed)
        {
            Python_ThreadBlock thread_block;
            Py_INCREF(py_object);
        }
    }
    PyObjectPtr(const PyObjectPtr &py_object_ptr)
    {
        Python_ThreadBlock thread_block;
        py_object = py_object_ptr.get();
        Py_INCREF(py_object);
    }
    ~PyObjectPtr()
    {
        Python_ThreadBlock thread_block;
        Py_XDECREF(this->py_object);
    }
    PyObjectPtr &operator=(const PyObjectPtr &) = delete;
    PyObject *get() const { return this->py_object; }
    PyObject *release()
    {
        PyObject *py_object = this->py_object;
        this->py_object = nullptr;
        return py_object;
    }
    bool isValid() const { return py_object != nullptr; }
    operator PyObject*() const { return py_object; }
    void setPyObject(PyObject *py_object)
    {
        Python_ThreadBlock thread_block;
        Py_XDECREF(this->py_object);
        Py_INCREF(py_object);
        this->py_object = py_object;
    }

private:
    PyObject *py_object;
};

struct PythonValueVariant {
    std::variant<
        std::nullptr_t,
        bool,
        long,
        long long,
        double,
        void *,
        std::string,
        std::map<std::string, PythonValueVariant>,
        std::vector<PythonValueVariant>,
        PyObjectPtr
    > value;
};

PythonValueVariant PyObjectToValueVariant(PyObject *py_object);
PyObject *PythonValueVariantToPyObject(const PythonValueVariant &value_variant);

class PythonWChar
{
    wchar_t *_s;
    Py_ssize_t _size;

public:
    PythonWChar(PyObject *o);
    ~PythonWChar();

    wchar_t *s() const { return _s; }
    Py_ssize_t size() const { return _size; }
};

class ImageInterface;

typedef PyObject *CreateAndAddModuleFn();

class FileSystem;

class PlatformSupport;

class PythonSupport
{
public:
    typedef int (*PyArg_ParseTupleFn)(PyObject *, const char *, ...);
    typedef PyObject* (*Py_BuildValueFn)(const char *, ...);

    static PythonSupport *instance();

    // only called once
    static void initInstance(FileSystem *fs, const std::string &python_home, const std::string &python_library);
    static void deinitInstance();

    static std::string ensurePython(FileSystem *fs, const std::string &python_home);

    void initialize(const std::string &python_home, const std::list<std::string> &python_paths, const std::string &python_library);
    void deinitialize();
    void addResourcePath(const std::string &resources_path);
    void imageFromRGBA(PyObject *ndarray_py, ImageInterface *image);
    void scaledImageFromRGBA(PyObject *ndarray_py, unsigned int width, unsigned int height, ImageInterface *image);
    void imageFromArray(PyObject *ndarray_py, float display_limit_low, float display_limit_high, PyObject *lookup_table, ImageInterface *image);
    void scaledImageFromArray(PyObject *ndarray_py, float width, float height, float context_scaling, float display_limit_low, float display_limit_high, PyObject *lookup_table, ImageInterface *image);
    PyObject *arrayFromImage(const ImageInterface &image);
    void bufferRelease(Py_buffer *buffer);
    PythonValueVariant invokePyMethod(PyObjectPtr *object, const std::string &method, const std::list<PythonValueVariant> &args);
    bool setAttribute(PyObjectPtr *object, const std::string &attribute, const PythonValueVariant &value);
    PythonValueVariant getAttribute(PyObjectPtr *object, const std::string &attribute);
    void setErrorString(const std::string &error_string);
    PyObject *getPyListFromStrings(const std::vector<std::string> &strings);
    PyArg_ParseTupleFn parse();
    Py_BuildValueFn build();
    PyObject *getNoneReturnValue();
    bool isNone(PyObject *obj);
    PyObject *createAndAddModule(PyModuleDef *moduledef);
    void prepareModuleException(const char *name);
    void initializeModule(const char *name, CreateAndAddModuleFn fn);
    void printAndClearErrors();
    PyObject *import(const char *name);
    void *UnwrapObject(PyObject *py_object);
    static const char* qobject_capsule_name;

    bool isValid() const { return m_valid; }
private:
    PythonSupport(FileSystem *fs, const std::string &python_home, const std::string &python_library); // ctor hidden
    PythonSupport(PythonSupport const&); // copy ctor hidden
    PythonSupport& operator=(PythonSupport const&); // assign op. hidden
    ~PythonSupport(); // dtor hidden

    // store the initial GIL state. the tool runs without holding the GIL, which is released after Python
    // is initialized. this variable allows restoration of the GIL when finalizing (exiting) the application.
    PyThreadState *m_initial_state;

    // actual python home after following venv
    std::string m_actual_python_home;

    // file system
    std::unique_ptr<FileSystem> fs;

    // platform support
    std::unique_ptr<PlatformSupport> ps;

    // whether dl loaded
    bool m_valid;

    // dynamics
    PyArg_ParseTupleFn dynamic_PyArg_ParseTuple;
    Py_BuildValueFn dynamic_Py_BuildValue;

    // exceptions
    PyObject *module_exception;
};

#endif // PYTHON_SUPPORT_H
