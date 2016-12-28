// Define the dynamic python methods, if in use.

#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON

#include <QtCore/QObject>

#if !defined(Q_OS_WIN)
#include <dlfcn.h>
#define LOOKUP_SYMBOL dlsym
#else
#include <Windows.h>
#include <WinBase.h>
void *LOOKUP_SYMBOL(void *h, const char *proc)
{
    void *addr = GetProcAddress(HMODULE(h), proc);
    Q_ASSERT(addr != 0);
    return addr;
}
#endif

#pragma push_macro("_DEBUG")
#undef _DEBUG
#include <Python.h>
#pragma pop_macro("_DEBUG")

#include "PythonStubs.h"

static void *pylib = 0;

void initialize_pylib(void *dl)
{
    pylib = dl;
}

bool DPyBool_Check(PyObject *o)
{
    PyTypeObject *DPyBool_Type = (PyTypeObject *)LOOKUP_SYMBOL(pylib, "PyBool_Type");
    return Py_TYPE(o) == DPyBool_Type;
}

bool DPyCapsule_CheckExact(PyObject *o)
{
    PyTypeObject *DPyCapsule_Type = (PyTypeObject *)LOOKUP_SYMBOL(pylib, "PyCapsule_Type");
    return Py_TYPE(o) == DPyCapsule_Type;
}

bool DPyFloat_Check(PyObject *o)
{
    PyTypeObject *DPyFloat_Type = (PyTypeObject *)LOOKUP_SYMBOL(pylib, "PyFloat_Type");
    return Py_TYPE(o) == DPyFloat_Type;
}

bool DPyModule_Check(PyObject *o)
{
    PyTypeObject *DPyModule_Type = (PyTypeObject *)LOOKUP_SYMBOL(pylib, "PyModule_Type");
    return Py_TYPE(o) == DPyModule_Type;
}

PyObject *DPyExc_GetAttributeError()
{
    return (PyObject *)LOOKUP_SYMBOL(pylib, "PyExc_AttributeError");
}

PyObject *DPyExc_GetImportError()
{
    return (PyObject *)LOOKUP_SYMBOL(pylib, "PyExc_ImportError");
}

PyObject *DPyExc_GetRuntimeError()
{
    return (PyObject *)LOOKUP_SYMBOL(pylib, "PyExc_RuntimeError");
}

PyObject *DPyExc_GetValueError()
{
    return (PyObject *)LOOKUP_SYMBOL(pylib, "PyExc_ValueError");
}

PyObject *DPy_TrueGet()
{
    return (PyObject *)LOOKUP_SYMBOL(pylib, "_Py_TrueStruct");
}

PyObject *DPy_FalseGet()
{
    return (PyObject *)LOOKUP_SYMBOL(pylib, "_Py_FalseStruct");
}

PyObject *DPy_NoneGet()
{
    return (PyObject *)LOOKUP_SYMBOL(pylib, "_Py_NoneStruct");
}

typedef int (*PyCallable_CheckFn)(PyObject *o);
int DPyCallable_Check(PyObject *o)
{
    static PyCallable_CheckFn f = 0;
    if (f == 0)
        f = (PyCallable_CheckFn)LOOKUP_SYMBOL(pylib, "PyCallable_Check");
    return f(o);
}

typedef void* (*PyCapsule_GetPointerFn)(PyObject *capsule, const char *name);
void* DPyCapsule_GetPointer(PyObject *capsule, const char *name)
{
    static PyCapsule_GetPointerFn f = 0;
    if (f == 0)
        f = (PyCapsule_GetPointerFn)LOOKUP_SYMBOL(pylib, "PyCapsule_GetPointer");
    return f(capsule, name);
}

typedef int (*PyCapsule_IsValidFn)(PyObject *capsule, const char *name);
int DPyCapsule_IsValid(PyObject *capsule, const char *name)
{
    static PyCapsule_IsValidFn f = 0;
    if (f == 0)
        f = (PyCapsule_IsValidFn)LOOKUP_SYMBOL(pylib, "PyCapsule_IsValid");
    return f(capsule, name);
}

typedef PyObject* (*PyCapsule_NewFn)(void *pointer, const char *name, PyCapsule_Destructor destructor);
PyObject* DPyCapsule_New(void *pointer, const char *name, PyCapsule_Destructor destructor)
{
    static PyCapsule_NewFn f = 0;
    if (f == 0)
        f = (PyCapsule_NewFn)LOOKUP_SYMBOL(pylib, "PyCapsule_New");
    return f(pointer, name, destructor);
}

typedef PyObject* (*PyDict_GetItemStringFn)(PyObject *p, const char *key);
PyObject* DPyDict_GetItemString(PyObject *p, const char *key)
{
    static PyDict_GetItemStringFn f = 0;
    if (f == 0)
        f = (PyDict_GetItemStringFn)LOOKUP_SYMBOL(pylib, "PyDict_GetItemString");
    return f(p, key);
}

typedef PyObject* (*PyDict_NewFn)();
PyObject* DPyDict_New()
{
    static PyDict_NewFn f = 0;
    if (f == 0)
        f = (PyDict_NewFn)LOOKUP_SYMBOL(pylib, "PyDict_New");
    return f();
}

typedef int (*PyDict_SetItemFn)(PyObject *p, PyObject *key, PyObject *val);
int DPyDict_SetItem(PyObject *p, PyObject *key, PyObject *val)
{
    static PyDict_SetItemFn f = 0;
    if (f == 0)
        f = (PyDict_SetItemFn)LOOKUP_SYMBOL(pylib, "PyDict_SetItem");
    return f(p, key, val);
}

typedef void (*PyErr_ClearFn)();
void DPyErr_Clear()
{
    static PyErr_ClearFn f = 0;
    if (f == 0)
        f = (PyErr_ClearFn)LOOKUP_SYMBOL(pylib, "PyErr_Clear");
    f();
}

typedef PyObject* (*PyErr_FormatFn)(PyObject *exception, const char *format, ...);
PyObject* DPyErr_Format(PyObject *exception, const char *format, ...)
{
    static PyErr_FormatFn f = 0;
    if (f == 0)
        f = (PyErr_FormatFn)LOOKUP_SYMBOL(pylib, "PyErr_Format");
    return 0;  // Numpy. Sheesh.
}

typedef PyObject* (*PyErr_OccurredFn)();
PyObject* DPyErr_Occurred()
{
    static PyErr_OccurredFn f = 0;
    if (f == 0)
        f = (PyErr_OccurredFn)LOOKUP_SYMBOL(pylib, "PyErr_Occurred");
    return f();
}

typedef void (*PyErr_PrintFn)();
void DPyErr_Print()
{
    static PyErr_PrintFn f = 0;
    if (f == 0)
        f = (PyErr_PrintFn)LOOKUP_SYMBOL(pylib, "PyErr_Print");
    f();
}

typedef PyObject *(*PyErr_NewExceptionFn)(const char *, PyObject *, PyObject *);
PyObject *DPyErr_NewException(const char *message, PyObject *base, PyObject *dict)
{
    static PyErr_NewExceptionFn f = 0;
    if (f == 0)
        f = (PyErr_NewExceptionFn)LOOKUP_SYMBOL(pylib, "PyErr_NewException");
    return f(message, base, dict);
}

typedef void (*PyErr_SetStringFn)(PyObject *type, const char *message);
void DPyErr_SetString(PyObject *type, const char *message)
{
    static PyErr_SetStringFn f = 0;
    if (f == 0)
        f = (PyErr_SetStringFn)LOOKUP_SYMBOL(pylib, "PyErr_SetString");
    return f(type, message);
}

typedef PyObject* (*PyEval_EvalCodeFn)(PyObject *co, PyObject *globals, PyObject *locals);
PyObject* DPyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    static PyEval_EvalCodeFn f = 0;
    if (f == 0)
        f = (PyEval_EvalCodeFn)LOOKUP_SYMBOL(pylib, "PyEval_EvalCode");
    return f(co, globals, locals);
}

typedef void (*PyEval_InitThreadsFn)();
void DPyEval_InitThreads()
{
    static PyEval_InitThreadsFn f = 0;
    if (f == 0)
        f = (PyEval_InitThreadsFn)LOOKUP_SYMBOL(pylib, "PyEval_InitThreads");
    f();
}

typedef void (*PyEval_RestoreThreadFn)(PyThreadState *tstate);
void DPyEval_RestoreThread(PyThreadState *tstate)
{
    static PyEval_RestoreThreadFn f = 0;
    if (f == 0)
        f = (PyEval_RestoreThreadFn)LOOKUP_SYMBOL(pylib, "PyEval_RestoreThread");
    f(tstate);
}

typedef PyThreadState* (*PyEval_SaveThreadFn)();
PyThreadState* DPyEval_SaveThread()
{
    static PyEval_SaveThreadFn f = 0;
    if (f == 0)
        f = (PyEval_SaveThreadFn)LOOKUP_SYMBOL(pylib, "PyEval_SaveThread");
    return f();
}

typedef double (*DPyFloat_AsDoubleFn)(PyObject *o);
double DPyFloat_AsDouble(PyObject *o)
{
    static DPyFloat_AsDoubleFn f = 0;
    if (f == 0)
        f = (DPyFloat_AsDoubleFn)LOOKUP_SYMBOL(pylib, "PyFloat_AsDouble");
    return f(o);
}

typedef PyObject* (*PyFloat_FromDoubleFn)(double v);
PyObject* DPyFloat_FromDouble(double v)
{
    static PyFloat_FromDoubleFn f = 0;
    if (f == 0)
        f = (PyFloat_FromDoubleFn)LOOKUP_SYMBOL(pylib, "PyFloat_FromDouble");
    return f(v);
}

typedef PyGILState_STATE (*PyGILState_EnsureFn)();
PyGILState_STATE DPyGILState_Ensure()
{
    static PyGILState_EnsureFn f = 0;
    if (f == 0)
        f = (PyGILState_EnsureFn)LOOKUP_SYMBOL(pylib, "PyGILState_Ensure");
    return f();
}

typedef int (*PyGILState_CheckFn)();
int DPyGILState_Check()
{
    static PyGILState_CheckFn f = 0;
    if (f == 0)
        f = (PyGILState_CheckFn)LOOKUP_SYMBOL(pylib, "PyGILState_Check");
    return f();
}

typedef void (*PyGILState_ReleaseFn)(PyGILState_STATE);
void DPyGILState_Release(PyGILState_STATE s)
{
    static PyGILState_ReleaseFn f = 0;
    if (f == 0)
        f = (PyGILState_ReleaseFn)LOOKUP_SYMBOL(pylib, "PyGILState_Release");
    f(s);
}

typedef int (*PyImport_AppendInittabFn)(const char *name, PyImport_AppendInittabInitFn initfunc);
int DPyImport_AppendInittab(const char *name, PyImport_AppendInittabInitFn initfunc)
{
    static PyImport_AppendInittabFn f = 0;
    if (f == 0)
        f = (PyImport_AppendInittabFn)LOOKUP_SYMBOL(pylib, "PyImport_AppendInittab");
    return f(name, initfunc);
}

typedef PyObject* (*PyImport_GetModuleDictFn)();
PyObject* DPyImport_GetModuleDict()
{
    static PyImport_GetModuleDictFn f = 0;
    if (f == 0)
        f = (PyImport_GetModuleDictFn)LOOKUP_SYMBOL(pylib, "PyImport_GetModuleDict");
    return f();
}

typedef PyObject* (*PyImport_ImportModuleFn)(const char *name);
PyObject* DPyImport_ImportModule(const char *name)
{
    static PyImport_ImportModuleFn f = 0;
    if (f == 0)
        f = (PyImport_ImportModuleFn)LOOKUP_SYMBOL(pylib, "PyImport_ImportModule");
    return f(name);
}

typedef int (*PyList_AppendFn)(PyObject *list, PyObject *item);
int DPyList_Append(PyObject *list, PyObject *item)
{
    static PyList_AppendFn f = 0;
    if (f == 0)
        f = (PyList_AppendFn)LOOKUP_SYMBOL(pylib, "PyList_Append");
    return f(list, item);
}

typedef PyObject* (*PyList_GetItemFn)(PyObject *list, Py_ssize_t index);
PyObject* DPyList_GetItem(PyObject *list, Py_ssize_t index)
{
    static PyList_GetItemFn f = 0;
    if (f == 0)
        f = (PyList_GetItemFn)LOOKUP_SYMBOL(pylib, "PyList_GetItem");
    return f(list, index);
}

typedef int (*PyList_InsertFn)(PyObject *list, Py_ssize_t index, PyObject *item);
int DPyList_Insert(PyObject *list, Py_ssize_t index, PyObject *item)
{
    static PyList_InsertFn f = 0;
    if (f == 0)
        f = (PyList_InsertFn)LOOKUP_SYMBOL(pylib, "PyList_Insert");
    return f(list, index, item);
}

typedef PyObject* (*PyList_NewFn)(Py_ssize_t len);
PyObject* DPyList_New(Py_ssize_t len)
{
    static PyList_NewFn f = 0;
    if (f == 0)
        f = (PyList_NewFn)LOOKUP_SYMBOL(pylib, "PyList_New");
    return f(len);
}

typedef Py_ssize_t (*PyList_SizeFn)(PyObject *list);
Py_ssize_t DPyList_Size(PyObject *list)
{
    static PyList_SizeFn f = 0;
    if (f == 0)
        f = (PyList_SizeFn)LOOKUP_SYMBOL(pylib, "PyList_Size");
    return f(list);
}

typedef long (*PyLong_AsLongFn)(PyObject *obj);
long DPyLong_AsLong(PyObject *obj)
{
    static PyLong_AsLongFn f = 0;
    if (f == 0)
        f = (PyLong_AsLongFn)LOOKUP_SYMBOL(pylib, "PyLong_AsLong");
    return f(obj);
}

typedef PY_LONG_LONG (*PyLong_AsLongLongFn)(PyObject *obj);
PY_LONG_LONG DPyLong_AsLongLong(PyObject *obj)
{
    static PyLong_AsLongLongFn f = 0;
    if (f == 0)
        f = (PyLong_AsLongLongFn)LOOKUP_SYMBOL(pylib, "PyLong_AsLongLong");
    return f(obj);
}

typedef PyObject* (*PyLong_FromLongFn)(long v);
PyObject* DPyLong_FromLong(long v)
{
    static PyLong_FromLongFn f = 0;
    if (f == 0)
        f = (PyLong_FromLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromLong");
    return f(v);
}

typedef PyObject* (*PyLong_FromLongLongFn)(PY_LONG_LONG v);
PyObject* DPyLong_FromLongLong(PY_LONG_LONG v)
{
    static PyLong_FromLongLongFn f = 0;
    if (f == 0)
        f = (PyLong_FromLongLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromLongLong");
    return f(v);
}

typedef PyObject* (*PyLong_FromUnsignedLongFn)(unsigned long v);
PyObject* DPyLong_FromUnsignedLong(unsigned long v)
{
    static PyLong_FromUnsignedLongFn f = 0;
    if (f == 0)
        f = (PyLong_FromUnsignedLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromUnsignedLong");
    return f(v);
}

typedef PyObject* (*PyLong_FromUnsignedLongLongFn)(unsigned PY_LONG_LONG v);
PyObject* DPyLong_FromUnsignedLongLong(unsigned PY_LONG_LONG v)
{
    static PyLong_FromUnsignedLongLongFn f = 0;
    if (f == 0)
        f = (PyLong_FromUnsignedLongLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromUnsignedLongLong");
    return f(v);
}

typedef int (*PyMapping_CheckFn)(PyObject *o);
int DPyMapping_Check(PyObject *o)
{
    static PyMapping_CheckFn f = 0;
    if (f == 0)
        f = (PyMapping_CheckFn)LOOKUP_SYMBOL(pylib, "PyMapping_Check");
    return f(o);
}

typedef PyObject* (*PyMapping_ItemsFn)(PyObject *o);
PyObject* DPyMapping_Items(PyObject *o)
{
    static PyMapping_ItemsFn f = 0;
    if (f == 0)
        f = (PyMapping_ItemsFn)LOOKUP_SYMBOL(pylib, "PyMapping_Items");
    return f(o);
}

typedef int (*PyModule_AddObjectFn)(PyObject *module, const char *name, PyObject *value);
int DPyModule_AddObject(PyObject *module, const char *name, PyObject *value)
{
    static PyModule_AddObjectFn f = 0;
    if (f == 0)
        f = (PyModule_AddObjectFn)LOOKUP_SYMBOL(pylib, "PyModule_AddObject");
    return f(module, name, value);
}

typedef PyObject* (*PyModule_Create2Fn)(PyModuleDef *module, int module_api_version);
PyObject* DPyModule_Create2(PyModuleDef *module, int module_api_version)
{
    static PyModule_Create2Fn f = 0;
    if (f == 0)
        f = (PyModule_Create2Fn)LOOKUP_SYMBOL(pylib, "PyModule_Create2");
    return f(module, module_api_version);
}

typedef PyObject* (*PyModule_GetDictFn)(PyObject *module);
PyObject* DPyModule_GetDict(PyObject *module)
{
    static PyModule_GetDictFn f = 0;
    if (f == 0)
        f = (PyModule_GetDictFn)LOOKUP_SYMBOL(pylib, "PyModule_GetDict");
    return f(module);
}

typedef PyObject* (*PyObject_CallObjectFn)(PyObject *callable_object, PyObject *args);
PyObject* DPyObject_CallObject(PyObject *callable_object, PyObject *args)
{
    static PyObject_CallObjectFn f = 0;
    if (f == 0)
        f = (PyObject_CallObjectFn)LOOKUP_SYMBOL(pylib, "PyObject_CallObject");
    return f(callable_object, args);
}

typedef PyObject* (*PyObject_GetAttrFn)(PyObject *o, PyObject *attr_name);
PyObject* DPyObject_GetAttr(PyObject *o, PyObject *attr_name)
{
    static PyObject_GetAttrFn f = 0;
    if (f == 0)
        f = (PyObject_GetAttrFn)LOOKUP_SYMBOL(pylib, "PyObject_GetAttr");
    return f(o, attr_name);
}

typedef PyObject* (*PyObject_GetAttrStringFn)(PyObject *o, const char *attr_name);
PyObject* DPyObject_GetAttrString(PyObject *o, const char *attr_name)
{
    static PyObject_GetAttrStringFn f = 0;
    if (f == 0)
        f = (PyObject_GetAttrStringFn)LOOKUP_SYMBOL(pylib, "PyObject_GetAttrString");
    return f(o, attr_name);
}

typedef int (*PyObject_IsTrueFn)(PyObject *o);
int DPyObject_IsTrue(PyObject *o)
{
    static PyObject_IsTrueFn f = 0;
    if (f == 0)
        f = (PyObject_IsTrueFn)LOOKUP_SYMBOL(pylib, "PyObject_IsTrue");
    return f(o);
}

typedef int (*PyObject_SetAttrFn)(PyObject *o, PyObject *attr_name, PyObject *v);
int DPyObject_SetAttr(PyObject *o, PyObject *attr_name, PyObject *v)
{
    static PyObject_SetAttrFn f = 0;
    if (f == 0)
        f = (PyObject_SetAttrFn)LOOKUP_SYMBOL(pylib, "PyObject_SetAttr");
    return f(o, attr_name, v);
}

typedef PyObject* (*PyRun_SimpleStringFn)(const char *str);
PyObject* DPyRun_SimpleString(const char *str)
{
    static PyRun_SimpleStringFn f = 0;
    if (f == 0)
        f = (PyRun_SimpleStringFn)LOOKUP_SYMBOL(pylib, "PyRun_SimpleString");
    return f(str);
}

typedef PyObject* (*PyRun_StringFlagsFn)(const char *str, int start, PyObject *globals, PyObject *locals, PyCompilerFlags *flags);
PyObject* DPyRun_StringFlags(const char *str, int start, PyObject *globals, PyObject *locals, PyCompilerFlags *flags)
{
    static PyRun_StringFlagsFn f = 0;
    if (f == 0)
        f = (PyRun_StringFlagsFn)LOOKUP_SYMBOL(pylib, "PyRun_StringFlags");
    return f(str, start, globals, locals, flags);
}

typedef int (*PySequence_CheckFn)(PyObject *o);
int DPySequence_Check(PyObject *o)
{
    static PySequence_CheckFn f = 0;
    if (f == 0)
        f = (PySequence_CheckFn)LOOKUP_SYMBOL(pylib, "PySequence_Check");
    return f(o);
}

typedef PyObject* (*PySequence_FastFn)(PyObject *o, const char *m);
PyObject* DPySequence_Fast(PyObject *o, const char *m)
{
    static PySequence_FastFn f = 0;
    if (f == 0)
        f = (PySequence_FastFn)LOOKUP_SYMBOL(pylib, "PySequence_Fast");
    return f(o, m);
}

typedef PyObject* (*PySequence_GetItemFn)(PyObject *o, Py_ssize_t i);
PyObject* DPySequence_GetItem(PyObject *o, Py_ssize_t i)
{
    static PySequence_GetItemFn f = 0;
    if (f == 0)
        f = (PySequence_GetItemFn)LOOKUP_SYMBOL(pylib, "PySequence_GetItem");
    return f(o, i);
}

typedef Py_ssize_t (*PySequence_SizeFn)(PyObject *o);
Py_ssize_t DPySequence_Size(PyObject *o)
{
    static PySequence_SizeFn f = 0;
    if (f == 0)
        f = (PySequence_SizeFn)LOOKUP_SYMBOL(pylib, "PySequence_Size");
    return f(o);
}

typedef int (*PyState_AddModuleFn)(PyObject *module, PyModuleDef *def);
int DPyState_AddModule(PyObject *module, PyModuleDef *def)
{
    static PyState_AddModuleFn f = 0;
    if (f == 0)
        f = (PyState_AddModuleFn)LOOKUP_SYMBOL(pylib, "PyState_AddModule");
    return f(module, def);
}

typedef PyObject* (*PyTuple_GetItemFn)(PyObject *p, Py_ssize_t pos);
PyObject* DPyTuple_GetItem(PyObject *p, Py_ssize_t pos)
{
    static PyTuple_GetItemFn f = 0;
    if (f == 0)
        f = (PyTuple_GetItemFn)LOOKUP_SYMBOL(pylib, "PyTuple_GetItem");
    return f(p, pos);
}

typedef PyObject* (*PyTuple_NewFn)(Py_ssize_t len);
PyObject* DPyTuple_New(Py_ssize_t len)
{
    static PyTuple_NewFn f = 0;
    if (f == 0)
        f = (PyTuple_NewFn)LOOKUP_SYMBOL(pylib, "PyTuple_New");
    return f(len);
}

typedef int (*PyTuple_SetItemFn)(PyObject *p, Py_ssize_t pos, PyObject *o);
int DPyTuple_SetItem(PyObject *p, Py_ssize_t pos, PyObject *o)
{
    static PyTuple_SetItemFn f = 0;
    if (f == 0)
        f = (PyTuple_SetItemFn)LOOKUP_SYMBOL(pylib, "PyTuple_SetItem");
    return f(p, pos, o);
}

typedef int (*PyType_IsSubtypeFn)(PyTypeObject *a, PyTypeObject *b);
int DPyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)
{
    static PyType_IsSubtypeFn f = 0;
    if (f == 0)
        f = (PyType_IsSubtypeFn)LOOKUP_SYMBOL(pylib, "PyType_IsSubtype");
    return f(a, b);
}

typedef char* (*PyUnicode_AsUTF8Fn)(PyObject *unicode);
char* DPyUnicode_AsUTF8(PyObject *unicode)
{
    static PyUnicode_AsUTF8Fn f = 0;
    if (f == 0)
        f = (PyUnicode_AsUTF8Fn)LOOKUP_SYMBOL(pylib, "PyUnicode_AsUTF8");
    return f(unicode);
}

typedef PyObject* (*PyUnicode_DecodeUTF16Fn)(const char *s, Py_ssize_t size, const char *errors, int *byteorder);
PyObject* DPyUnicode_DecodeUTF16(const char *s, Py_ssize_t size, const char *errors, int *byteorder)
{
    static PyUnicode_DecodeUTF16Fn f = 0;
    if (f == 0)
        f = (PyUnicode_DecodeUTF16Fn)LOOKUP_SYMBOL(pylib, "PyUnicode_DecodeUTF16");
    return f(s, size, errors, byteorder);
}

typedef PyObject *(*PyUnicode_FromStringFn)(const char *u);
PyObject* DPyUnicode_FromString(const char *u)
{
    static PyUnicode_FromStringFn f = 0;
    if (f == 0)
        f = (PyUnicode_FromStringFn)LOOKUP_SYMBOL(pylib, "PyUnicode_FromString");
    return f(u);
}

typedef PyObject* (*Py_CompileStringExFlagsFn)(const char *str, const char *filename, int start, PyCompilerFlags *flags, int optimize);
PyObject* DPy_CompileStringExFlags(const char *str, const char *filename, int start, PyCompilerFlags *flags, int optimize)
{
    static Py_CompileStringExFlagsFn f = 0;
    if (f == 0)
        f = (Py_CompileStringExFlagsFn)LOOKUP_SYMBOL(pylib, "Py_CompileStringExFlags");
    return f(str, filename, start, flags, optimize);
}

typedef void (*Py_InitializeFn)();
void DPy_Initialize()
{
    static Py_InitializeFn f = 0;
    if (f == 0)
        f = (Py_InitializeFn)LOOKUP_SYMBOL(pylib, "Py_Initialize");
    return f();
}

typedef void(*Py_SetPythonHomeFn)(wchar_t *ph);
void DPy_SetPythonHome(wchar_t *ph)
{
    static Py_SetPythonHomeFn f = 0;
    if (f == 0)
        f = (Py_SetPythonHomeFn)LOOKUP_SYMBOL(pylib, "Py_SetPythonHome");
    return f(ph);
}

#endif // defined(DYNAMIC_PYTHON)

