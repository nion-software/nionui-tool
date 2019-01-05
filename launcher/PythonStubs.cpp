// Define the dynamic python methods, if in use.

#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON

#include <QtCore/QObject>
#include <QtCore/QDebug>

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
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#pragma pop_macro("_DEBUG")

#include "PythonStubs.h"

void *pylib = 0;

typedef int (*PyBuffer_ReleaseFn)(Py_buffer *o);
typedef int (*PyCallable_CheckFn)(PyObject *o);
typedef void* (*PyCapsule_GetPointerFn)(PyObject *capsule, const char *name);
typedef int (*PyCapsule_IsValidFn)(PyObject *capsule, const char *name);
typedef PyObject* (*PyCapsule_NewFn)(void *pointer, const char *name, PyCapsule_Destructor destructor);
typedef PyObject* (*PyDict_GetItemStringFn)(PyObject *p, const char *key);
typedef PyObject* (*PyDict_NewFn)();
typedef int (*PyDict_SetItemFn)(PyObject *p, PyObject *key, PyObject *val);
typedef void (*PyErr_ClearFn)();
typedef PyObject* (*PyErr_FormatFn)(PyObject *exception, const char *format, ...);
typedef PyObject* (*PyErr_OccurredFn)();
typedef void (*PyErr_PrintFn)();
typedef PyObject *(*PyErr_NewExceptionFn)(const char *, PyObject *, PyObject *);
typedef void (*PyErr_SetStringFn)(PyObject *type, const char *message);
typedef PyObject* (*PyEval_EvalCodeFn)(PyObject *co, PyObject *globals, PyObject *locals);
typedef void (*PyEval_InitThreadsFn)();
typedef void (*PyEval_RestoreThreadFn)(PyThreadState *tstate);
typedef PyThreadState* (*PyEval_SaveThreadFn)();
typedef double (*DPyFloat_AsDoubleFn)(PyObject *o);
typedef PyObject* (*PyFloat_FromDoubleFn)(double v);
typedef PyGILState_STATE (*PyGILState_EnsureFn)();
typedef int (*PyGILState_CheckFn)();
typedef void (*PyGILState_ReleaseFn)(PyGILState_STATE);
typedef int (*PyImport_AppendInittabFn)(const char *name, PyImport_AppendInittabInitFn initfunc);
typedef PyObject* (*PyImport_GetModuleDictFn)();
typedef PyObject* (*PyImport_ImportModuleFn)(const char *name);
typedef int (*PyList_AppendFn)(PyObject *list, PyObject *item);
typedef PyObject* (*PyList_GetItemFn)(PyObject *list, Py_ssize_t index);
typedef int (*PyList_InsertFn)(PyObject *list, Py_ssize_t index, PyObject *item);
typedef PyObject* (*PyList_NewFn)(Py_ssize_t len);
typedef Py_ssize_t (*PyList_SizeFn)(PyObject *list);
typedef long (*PyLong_AsLongFn)(PyObject *obj);
typedef PY_LONG_LONG (*PyLong_AsLongLongFn)(PyObject *obj);
typedef PyObject* (*PyLong_FromLongFn)(long v);
typedef PyObject* (*PyLong_FromLongLongFn)(PY_LONG_LONG v);
typedef PyObject* (*PyLong_FromUnsignedLongFn)(unsigned long v);
typedef PyObject* (*PyLong_FromUnsignedLongLongFn)(unsigned PY_LONG_LONG v);
typedef int (*PyMapping_CheckFn)(PyObject *o);
typedef PyObject* (*PyMapping_ItemsFn)(PyObject *o);
typedef int (*PyModule_AddObjectFn)(PyObject *module, const char *name, PyObject *value);
typedef PyObject* (*PyModule_Create2Fn)(PyModuleDef *module, int module_api_version);
typedef PyObject* (*PyModule_GetDictFn)(PyObject *module);
typedef PyObject* (*PyObject_CallObjectFn)(PyObject *callable_object, PyObject *args);
typedef PyObject* (*PyObject_GetAttrFn)(PyObject *o, PyObject *attr_name);
typedef PyObject* (*PyObject_GetAttrStringFn)(PyObject *o, const char *attr_name);
typedef int (*PyObject_IsTrueFn)(PyObject *o);
typedef int (*PyObject_SetAttrFn)(PyObject *o, PyObject *attr_name, PyObject *v);
typedef PyObject* (*PyRun_SimpleStringFn)(const char *str);
typedef PyObject* (*PyRun_StringFlagsFn)(const char *str, int start, PyObject *globals, PyObject *locals, PyCompilerFlags *flags);
typedef int (*PySequence_CheckFn)(PyObject *o);
typedef PyObject* (*PySequence_FastFn)(PyObject *o, const char *m);
typedef PyObject* (*PySequence_GetItemFn)(PyObject *o, Py_ssize_t i);
typedef Py_ssize_t (*PySequence_SizeFn)(PyObject *o);
typedef int (*PyState_AddModuleFn)(PyObject *module, PyModuleDef *def);
typedef PyObject* (*PyTuple_GetItemFn)(PyObject *p, Py_ssize_t pos);
typedef PyObject* (*PyTuple_NewFn)(Py_ssize_t len);
typedef int (*PyTuple_SetItemFn)(PyObject *p, Py_ssize_t pos, PyObject *o);
typedef int (*PyType_IsSubtypeFn)(PyTypeObject *a, PyTypeObject *b);
typedef char* (*PyUnicode_AsUTF8Fn)(PyObject *unicode);
typedef PyObject* (*PyUnicode_DecodeUTF16Fn)(const char *s, Py_ssize_t size, const char *errors, int *byteorder);
typedef PyObject *(*PyUnicode_FromStringFn)(const char *u);
typedef PyObject* (*Py_CompileStringExFlagsFn)(const char *str, const char *filename, int start, PyCompilerFlags *flags, int optimize);
typedef void (*Py_InitializeFn)();
typedef void (*Py_FinalizeFn)();
typedef void(*Py_SetPythonHomeFn)(wchar_t *ph);
typedef void(*Py_SetPathFn)(wchar_t *ph);
typedef void(*Py_SetProgramNameFn)(wchar_t *ph);

static PyBuffer_ReleaseFn fBuffer_Release = 0;
static PyCallable_CheckFn fCallable_Check = 0;
static PyCapsule_GetPointerFn fCapsule_GetPointer = 0;
static PyCapsule_IsValidFn fCapsule_IsValid = 0;
static PyCapsule_NewFn fCapsule_New = 0;
static PyDict_GetItemStringFn fDict_GetItemString = 0;
static PyDict_NewFn fDict_New = 0;
static PyDict_SetItemFn fDict_SetItem = 0;
static PyErr_ClearFn fErr_Clear = 0;
static PyErr_FormatFn fErr_Format = 0;
static PyErr_OccurredFn fErr_Occurred = 0;
static PyErr_PrintFn fErr_Print = 0;
static PyErr_NewExceptionFn fErr_NewException = 0;
static PyErr_SetStringFn fErr_SetString = 0;
static PyEval_EvalCodeFn fEval_EvalCode = 0;
static PyEval_InitThreadsFn fEval_InitThreads = 0;
static PyEval_RestoreThreadFn fEval_RestoreThread = 0;
static PyEval_SaveThreadFn fEval_SaveThread = 0;
static DPyFloat_AsDoubleFn fFloat_AsDouble = 0;
static PyFloat_FromDoubleFn fFloat_FromDouble = 0;
static PyGILState_EnsureFn fGILState_Ensure = 0;
static PyGILState_CheckFn fGILState_Check = 0;
static PyGILState_ReleaseFn fGILState_Release = 0;
static PyImport_AppendInittabFn fImport_AppendInittab = 0;
static PyImport_GetModuleDictFn fImport_GetModuleDict = 0;
static PyImport_ImportModuleFn fImport_ImportModule = 0;
static PyList_AppendFn fList_Append = 0;
static PyList_GetItemFn fList_GetItem = 0;
static PyList_InsertFn fList_Insert = 0;
static PyList_NewFn fList_New = 0;
static PyList_SizeFn fList_Size = 0;
static PyLong_AsLongFn fLong_AsLong = 0;
static PyLong_AsLongLongFn fLong_AsLongLong = 0;
static PyLong_FromLongFn fLong_FromLong = 0;
static PyLong_FromLongLongFn fLong_FromLongLong = 0;
static PyLong_FromUnsignedLongFn fLong_FromUnsignedLong = 0;
static PyLong_FromUnsignedLongLongFn fLong_FromUnsignedLongLong = 0;
static PyMapping_CheckFn fMapping_Check = 0;
static PyMapping_ItemsFn fMapping_Items = 0;
static PyModule_AddObjectFn fModule_AddObject = 0;
static PyModule_Create2Fn fModule_Create2 = 0;
static PyModule_GetDictFn fModule_GetDict = 0;
static PyObject_CallObjectFn fObject_CallObject = 0;
static PyObject_GetAttrFn fObject_GetAttr = 0;
static PyObject_GetAttrStringFn fObject_GetAttrString = 0;
static PyObject_IsTrueFn fObject_IsTrue = 0;
static PyObject_SetAttrFn fObject_SetAttr = 0;
static PyRun_SimpleStringFn fRun_SimpleString = 0;
static PyRun_StringFlagsFn fRun_StringFlags = 0;
static PySequence_CheckFn fSequence_Check = 0;
static PySequence_FastFn fSequence_Fast = 0;
static PySequence_GetItemFn fSequence_GetItem = 0;
static PySequence_SizeFn fSequence_Size = 0;
static PyState_AddModuleFn fState_AddModule = 0;
static PyTuple_GetItemFn fTuple_GetItem = 0;
static PyTuple_NewFn fTuple_New = 0;
static PyTuple_SetItemFn fTuple_SetItem = 0;
static PyType_IsSubtypeFn fType_IsSubtype = 0;
static PyUnicode_AsUTF8Fn fUnicode_AsUTF8 = 0;
static PyUnicode_DecodeUTF16Fn fUnicode_DecodeUTF16 = 0;
static PyUnicode_FromStringFn fUnicode_FromString = 0;
static Py_CompileStringExFlagsFn fCompileStringExFlags = 0;
static Py_InitializeFn fInitialize = 0;
static Py_FinalizeFn fFinalize = 0;
static Py_SetPythonHomeFn fSetPythonHome = 0;
static Py_SetPathFn fSetPath = 0;
static Py_SetProgramNameFn fSetProgramName = 0;

void initialize_pylib(void *dl)
{
    pylib = dl;
}

void deinitialize_pylib()
{
    pylib = nullptr;

    fBuffer_Release = 0;
    fCallable_Check = 0;
    fCapsule_GetPointer = 0;
    fCapsule_IsValid = 0;
    fCapsule_New = 0;
    fDict_GetItemString = 0;
    fDict_New = 0;
    fDict_SetItem = 0;
    fErr_Clear = 0;
    fErr_Format = 0;
    fErr_Occurred = 0;
    fErr_Print = 0;
    fErr_NewException = 0;
    fErr_SetString = 0;
    fEval_EvalCode = 0;
    fEval_InitThreads = 0;
    fEval_RestoreThread = 0;
    fEval_SaveThread = 0;
    fFloat_AsDouble = 0;
    fFloat_FromDouble = 0;
    fGILState_Ensure = 0;
    fGILState_Check = 0;
    fGILState_Release = 0;
    fImport_AppendInittab = 0;
    fImport_GetModuleDict = 0;
    fImport_ImportModule = 0;
    fList_Append = 0;
    fList_GetItem = 0;
    fList_Insert = 0;
    fList_New = 0;
    fList_Size = 0;
    fLong_AsLong = 0;
    fLong_AsLongLong = 0;
    fLong_FromLong = 0;
    fLong_FromLongLong = 0;
    fLong_FromUnsignedLong = 0;
    fLong_FromUnsignedLongLong = 0;
    fMapping_Check = 0;
    fMapping_Items = 0;
    fModule_AddObject = 0;
    fModule_Create2 = 0;
    fModule_GetDict = 0;
    fObject_CallObject = 0;
    fObject_GetAttr = 0;
    fObject_GetAttrString = 0;
    fObject_IsTrue = 0;
    fObject_SetAttr = 0;
    fRun_SimpleString = 0;
    fRun_StringFlags = 0;
    fSequence_Check = 0;
    fSequence_Fast = 0;
    fSequence_GetItem = 0;
    fSequence_Size = 0;
    fState_AddModule = 0;
    fTuple_GetItem = 0;
    fTuple_New = 0;
    fTuple_SetItem = 0;
    fType_IsSubtype = 0;
    fUnicode_AsUTF8 = 0;
    fUnicode_DecodeUTF16 = 0;
    fUnicode_FromString = 0;
    fCompileStringExFlags = 0;
    fInitialize = 0;
    fFinalize = 0;
    fSetPythonHome = 0;
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

void DPyBuffer_Release(Py_buffer *o)
{
    if (fBuffer_Release == 0)
        fBuffer_Release = (PyBuffer_ReleaseFn)LOOKUP_SYMBOL(pylib, "PyBuffer_Release");
    fBuffer_Release(o);
}

int DPyCallable_Check(PyObject *o)
{
    if (fCallable_Check == 0)
        fCallable_Check = (PyCallable_CheckFn)LOOKUP_SYMBOL(pylib, "PyCallable_Check");
    return fCallable_Check(o);
}

void* DPyCapsule_GetPointer(PyObject *capsule, const char *name)
{
    if (fCapsule_GetPointer == 0)
        fCapsule_GetPointer = (PyCapsule_GetPointerFn)LOOKUP_SYMBOL(pylib, "PyCapsule_GetPointer");
    return fCapsule_GetPointer(capsule, name);
}

int DPyCapsule_IsValid(PyObject *capsule, const char *name)
{
    if (fCapsule_IsValid == 0)
        fCapsule_IsValid = (PyCapsule_IsValidFn)LOOKUP_SYMBOL(pylib, "PyCapsule_IsValid");
    return fCapsule_IsValid(capsule, name);
}

PyObject* DPyCapsule_New(void *pointer, const char *name, PyCapsule_Destructor destructor)
{
    if (fCapsule_New == 0)
        fCapsule_New = (PyCapsule_NewFn)LOOKUP_SYMBOL(pylib, "PyCapsule_New");
    return fCapsule_New(pointer, name, destructor);
}

PyObject* DPyDict_GetItemString(PyObject *p, const char *key)
{
    if (fDict_GetItemString == 0)
        fDict_GetItemString = (PyDict_GetItemStringFn)LOOKUP_SYMBOL(pylib, "PyDict_GetItemString");
    return fDict_GetItemString(p, key);
}

PyObject* DPyDict_New()
{
    if (fDict_New == 0)
        fDict_New = (PyDict_NewFn)LOOKUP_SYMBOL(pylib, "PyDict_New");
    return fDict_New();
}

int DPyDict_SetItem(PyObject *p, PyObject *key, PyObject *val)
{
    if (fDict_SetItem == 0)
        fDict_SetItem = (PyDict_SetItemFn)LOOKUP_SYMBOL(pylib, "PyDict_SetItem");
    return fDict_SetItem(p, key, val);
}

void DPyErr_Clear()
{
    if (fErr_Clear == 0)
        fErr_Clear = (PyErr_ClearFn)LOOKUP_SYMBOL(pylib, "PyErr_Clear");
    fErr_Clear();
}

PyObject* DPyErr_Format(PyObject *exception, const char *format, ...)
{
    if (fErr_Format == 0)
        fErr_Format = (PyErr_FormatFn)LOOKUP_SYMBOL(pylib, "PyErr_Format");
    return 0;  // Numpy. Sheesh.
}

PyObject* DPyErr_Occurred()
{
    if (fErr_Occurred == 0)
        fErr_Occurred = (PyErr_OccurredFn)LOOKUP_SYMBOL(pylib, "PyErr_Occurred");
    return fErr_Occurred();
}

void DPyErr_Print()
{
    if (fErr_Print == 0)
        fErr_Print = (PyErr_PrintFn)LOOKUP_SYMBOL(pylib, "PyErr_Print");
    fErr_Print();
}

PyObject *DPyErr_NewException(const char *message, PyObject *base, PyObject *dict)
{
    if (fErr_NewException == 0)
        fErr_NewException = (PyErr_NewExceptionFn)LOOKUP_SYMBOL(pylib, "PyErr_NewException");
    return fErr_NewException(message, base, dict);
}

void DPyErr_SetString(PyObject *type, const char *message)
{
    if (fErr_SetString == 0)
        fErr_SetString = (PyErr_SetStringFn)LOOKUP_SYMBOL(pylib, "PyErr_SetString");
    return fErr_SetString(type, message);
}

PyObject* DPyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    if (fEval_EvalCode == 0)
        fEval_EvalCode = (PyEval_EvalCodeFn)LOOKUP_SYMBOL(pylib, "PyEval_EvalCode");
    return fEval_EvalCode(co, globals, locals);
}

void DPyEval_InitThreads()
{
    if (fEval_InitThreads == 0)
        fEval_InitThreads = (PyEval_InitThreadsFn)LOOKUP_SYMBOL(pylib, "PyEval_InitThreads");
    fEval_InitThreads();
}

void DPyEval_RestoreThread(PyThreadState *tstate)
{
    if (fEval_RestoreThread == 0)
        fEval_RestoreThread = (PyEval_RestoreThreadFn)LOOKUP_SYMBOL(pylib, "PyEval_RestoreThread");
    fEval_RestoreThread(tstate);
}

PyThreadState* DPyEval_SaveThread()
{
    if (fEval_SaveThread == 0)
        fEval_SaveThread = (PyEval_SaveThreadFn)LOOKUP_SYMBOL(pylib, "PyEval_SaveThread");
    return fEval_SaveThread();
}

double DPyFloat_AsDouble(PyObject *o)
{
    if (fFloat_AsDouble == 0)
        fFloat_AsDouble = (DPyFloat_AsDoubleFn)LOOKUP_SYMBOL(pylib, "PyFloat_AsDouble");
    return fFloat_AsDouble(o);
}

PyObject* DPyFloat_FromDouble(double v)
{
    if (fFloat_FromDouble == 0)
        fFloat_FromDouble = (PyFloat_FromDoubleFn)LOOKUP_SYMBOL(pylib, "PyFloat_FromDouble");
    return fFloat_FromDouble(v);
}

PyGILState_STATE DPyGILState_Ensure()
{
    if (fGILState_Ensure == 0)
        fGILState_Ensure = (PyGILState_EnsureFn)LOOKUP_SYMBOL(pylib, "PyGILState_Ensure");
    return fGILState_Ensure();
}

int DPyGILState_Check()
{
    if (fGILState_Check == 0)
        fGILState_Check = (PyGILState_CheckFn)LOOKUP_SYMBOL(pylib, "PyGILState_Check");
    return fGILState_Check();
}

void DPyGILState_Release(PyGILState_STATE s)
{
    if (fGILState_Release == 0)
        fGILState_Release = (PyGILState_ReleaseFn)LOOKUP_SYMBOL(pylib, "PyGILState_Release");
    fGILState_Release(s);
}

int DPyImport_AppendInittab(const char *name, PyImport_AppendInittabInitFn initfunc)
{
    if (fImport_AppendInittab == 0)
        fImport_AppendInittab = (PyImport_AppendInittabFn)LOOKUP_SYMBOL(pylib, "PyImport_AppendInittab");
    return fImport_AppendInittab(name, initfunc);
}

PyObject* DPyImport_GetModuleDict()
{
    if (fImport_GetModuleDict == 0)
        fImport_GetModuleDict = (PyImport_GetModuleDictFn)LOOKUP_SYMBOL(pylib, "PyImport_GetModuleDict");
    return fImport_GetModuleDict();
}

PyObject* DPyImport_ImportModule(const char *name)
{
    if (fImport_ImportModule == 0)
        fImport_ImportModule = (PyImport_ImportModuleFn)LOOKUP_SYMBOL(pylib, "PyImport_ImportModule");
    return fImport_ImportModule(name);
}

int DPyList_Append(PyObject *list, PyObject *item)
{
    if (fList_Append == 0)
        fList_Append = (PyList_AppendFn)LOOKUP_SYMBOL(pylib, "PyList_Append");
    return fList_Append(list, item);
}

PyObject* DPyList_GetItem(PyObject *list, Py_ssize_t index)
{
    if (fList_GetItem == 0)
        fList_GetItem = (PyList_GetItemFn)LOOKUP_SYMBOL(pylib, "PyList_GetItem");
    return fList_GetItem(list, index);
}

int DPyList_Insert(PyObject *list, Py_ssize_t index, PyObject *item)
{
    if (fList_Insert == 0)
        fList_Insert = (PyList_InsertFn)LOOKUP_SYMBOL(pylib, "PyList_Insert");
    return fList_Insert(list, index, item);
}

PyObject* DPyList_New(Py_ssize_t len)
{
    if (fList_New == 0)
        fList_New = (PyList_NewFn)LOOKUP_SYMBOL(pylib, "PyList_New");
    return fList_New(len);
}

Py_ssize_t DPyList_Size(PyObject *list)
{
    if (fList_Size == 0)
        fList_Size = (PyList_SizeFn)LOOKUP_SYMBOL(pylib, "PyList_Size");
    return fList_Size(list);
}

long DPyLong_AsLong(PyObject *obj)
{
    if (fLong_AsLong == 0)
        fLong_AsLong = (PyLong_AsLongFn)LOOKUP_SYMBOL(pylib, "PyLong_AsLong");
    return fLong_AsLong(obj);
}

PY_LONG_LONG DPyLong_AsLongLong(PyObject *obj)
{
    if (fLong_AsLongLong == 0)
        fLong_AsLongLong = (PyLong_AsLongLongFn)LOOKUP_SYMBOL(pylib, "PyLong_AsLongLong");
    return fLong_AsLongLong(obj);
}

PyObject* DPyLong_FromLong(long v)
{
    if (fLong_FromLong == 0)
        fLong_FromLong = (PyLong_FromLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromLong");
    return fLong_FromLong(v);
}

PyObject* DPyLong_FromLongLong(PY_LONG_LONG v)
{
    if (fLong_FromLongLong == 0)
        fLong_FromLongLong = (PyLong_FromLongLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromLongLong");
    return fLong_FromLongLong(v);
}

PyObject* DPyLong_FromUnsignedLong(unsigned long v)
{
    if (fLong_FromUnsignedLong == 0)
        fLong_FromUnsignedLong = (PyLong_FromUnsignedLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromUnsignedLong");
    return fLong_FromUnsignedLong(v);
}

PyObject* DPyLong_FromUnsignedLongLong(unsigned PY_LONG_LONG v)
{
    if (fLong_FromUnsignedLongLong == 0)
        fLong_FromUnsignedLongLong = (PyLong_FromUnsignedLongLongFn)LOOKUP_SYMBOL(pylib, "PyLong_FromUnsignedLongLong");
    return fLong_FromUnsignedLongLong(v);
}

int DPyMapping_Check(PyObject *o)
{
    if (fMapping_Check == 0)
        fMapping_Check = (PyMapping_CheckFn)LOOKUP_SYMBOL(pylib, "PyMapping_Check");
    return fMapping_Check(o);
}

PyObject* DPyMapping_Items(PyObject *o)
{
    if (fMapping_Items == 0)
        fMapping_Items = (PyMapping_ItemsFn)LOOKUP_SYMBOL(pylib, "PyMapping_Items");
    return fMapping_Items(o);
}

int DPyModule_AddObject(PyObject *module, const char *name, PyObject *value)
{
    if (fModule_AddObject == 0)
        fModule_AddObject = (PyModule_AddObjectFn)LOOKUP_SYMBOL(pylib, "PyModule_AddObject");
    return fModule_AddObject(module, name, value);
}

PyObject* DPyModule_Create2(PyModuleDef *module, int module_api_version)
{
    if (fModule_Create2 == 0)
        fModule_Create2 = (PyModule_Create2Fn)LOOKUP_SYMBOL(pylib, "PyModule_Create2");
    return fModule_Create2(module, module_api_version);
}

PyObject* DPyModule_GetDict(PyObject *module)
{
    if (fModule_GetDict == 0)
        fModule_GetDict = (PyModule_GetDictFn)LOOKUP_SYMBOL(pylib, "PyModule_GetDict");
    return fModule_GetDict(module);
}

PyObject* DPyObject_CallObject(PyObject *callable_object, PyObject *args)
{
    if (fObject_CallObject == 0)
        fObject_CallObject = (PyObject_CallObjectFn)LOOKUP_SYMBOL(pylib, "PyObject_CallObject");
    return fObject_CallObject(callable_object, args);
}

PyObject* DPyObject_GetAttr(PyObject *o, PyObject *attr_name)
{
    if (fObject_GetAttr == 0)
        fObject_GetAttr = (PyObject_GetAttrFn)LOOKUP_SYMBOL(pylib, "PyObject_GetAttr");
    return fObject_GetAttr(o, attr_name);
}

PyObject* DPyObject_GetAttrString(PyObject *o, const char *attr_name)
{
    if (fObject_GetAttrString == 0)
        fObject_GetAttrString = (PyObject_GetAttrStringFn)LOOKUP_SYMBOL(pylib, "PyObject_GetAttrString");
    return fObject_GetAttrString(o, attr_name);
}

int DPyObject_IsTrue(PyObject *o)
{
    if (fObject_IsTrue == 0)
        fObject_IsTrue = (PyObject_IsTrueFn)LOOKUP_SYMBOL(pylib, "PyObject_IsTrue");
    return fObject_IsTrue(o);
}

int DPyObject_SetAttr(PyObject *o, PyObject *attr_name, PyObject *v)
{
    if (fObject_SetAttr == 0)
        fObject_SetAttr = (PyObject_SetAttrFn)LOOKUP_SYMBOL(pylib, "PyObject_SetAttr");
    return fObject_SetAttr(o, attr_name, v);
}

PyObject* DPyRun_SimpleString(const char *str)
{
    if (fRun_SimpleString == 0)
        fRun_SimpleString = (PyRun_SimpleStringFn)LOOKUP_SYMBOL(pylib, "PyRun_SimpleString");
    return fRun_SimpleString(str);
}

PyObject* DPyRun_StringFlags(const char *str, int start, PyObject *globals, PyObject *locals, PyCompilerFlags *flags)
{
    if (fRun_StringFlags == 0)
        fRun_StringFlags = (PyRun_StringFlagsFn)LOOKUP_SYMBOL(pylib, "PyRun_StringFlags");
    return fRun_StringFlags(str, start, globals, locals, flags);
}

int DPySequence_Check(PyObject *o)
{
    if (fSequence_Check == 0)
        fSequence_Check = (PySequence_CheckFn)LOOKUP_SYMBOL(pylib, "PySequence_Check");
    return fSequence_Check(o);
}

PyObject* DPySequence_Fast(PyObject *o, const char *m)
{
    if (fSequence_Fast == 0)
        fSequence_Fast = (PySequence_FastFn)LOOKUP_SYMBOL(pylib, "PySequence_Fast");
    return fSequence_Fast(o, m);
}

PyObject* DPySequence_GetItem(PyObject *o, Py_ssize_t i)
{
    if (fSequence_GetItem == 0)
        fSequence_GetItem = (PySequence_GetItemFn)LOOKUP_SYMBOL(pylib, "PySequence_GetItem");
    return fSequence_GetItem(o, i);
}

Py_ssize_t DPySequence_Size(PyObject *o)
{
    if (fSequence_Size == 0)
        fSequence_Size = (PySequence_SizeFn)LOOKUP_SYMBOL(pylib, "PySequence_Size");
    return fSequence_Size(o);
}

int DPyState_AddModule(PyObject *module, PyModuleDef *def)
{
    if (fState_AddModule == 0)
        fState_AddModule = (PyState_AddModuleFn)LOOKUP_SYMBOL(pylib, "PyState_AddModule");
    return fState_AddModule(module, def);
}

PyObject* DPyTuple_GetItem(PyObject *p, Py_ssize_t pos)
{
    if (fTuple_GetItem == 0)
        fTuple_GetItem = (PyTuple_GetItemFn)LOOKUP_SYMBOL(pylib, "PyTuple_GetItem");
    return fTuple_GetItem(p, pos);
}

PyObject* DPyTuple_New(Py_ssize_t len)
{
    if (fTuple_New == 0)
        fTuple_New = (PyTuple_NewFn)LOOKUP_SYMBOL(pylib, "PyTuple_New");
    return fTuple_New(len);
}

int DPyTuple_SetItem(PyObject *p, Py_ssize_t pos, PyObject *o)
{
    if (fTuple_SetItem == 0)
        fTuple_SetItem = (PyTuple_SetItemFn)LOOKUP_SYMBOL(pylib, "PyTuple_SetItem");
    return fTuple_SetItem(p, pos, o);
}

int DPyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)
{
    if (fType_IsSubtype == 0)
        fType_IsSubtype = (PyType_IsSubtypeFn)LOOKUP_SYMBOL(pylib, "PyType_IsSubtype");
    return fType_IsSubtype(a, b);
}

char* DPyUnicode_AsUTF8(PyObject *unicode)
{
    if (fUnicode_AsUTF8 == 0)
        fUnicode_AsUTF8 = (PyUnicode_AsUTF8Fn)LOOKUP_SYMBOL(pylib, "PyUnicode_AsUTF8");
    return fUnicode_AsUTF8(unicode);
}

PyObject* DPyUnicode_DecodeUTF16(const char *s, Py_ssize_t size, const char *errors, int *byteorder)
{
    if (fUnicode_DecodeUTF16 == 0)
        fUnicode_DecodeUTF16 = (PyUnicode_DecodeUTF16Fn)LOOKUP_SYMBOL(pylib, "PyUnicode_DecodeUTF16");
    return fUnicode_DecodeUTF16(s, size, errors, byteorder);
}

PyObject* DPyUnicode_FromString(const char *u)
{
    if (fUnicode_FromString == 0)
        fUnicode_FromString = (PyUnicode_FromStringFn)LOOKUP_SYMBOL(pylib, "PyUnicode_FromString");
    return fUnicode_FromString(u);
}

PyObject* DPy_CompileStringExFlags(const char *str, const char *filename, int start, PyCompilerFlags *flags, int optimize)
{
    if (fCompileStringExFlags == 0)
        fCompileStringExFlags = (Py_CompileStringExFlagsFn)LOOKUP_SYMBOL(pylib, "Py_CompileStringExFlags");
    return fCompileStringExFlags(str, filename, start, flags, optimize);
}

void DPy_Initialize()
{
    if (fInitialize == 0)
        fInitialize = (Py_InitializeFn)LOOKUP_SYMBOL(pylib, "Py_Initialize");
    return fInitialize();
}

void DPy_Finalize()
{
    if (fFinalize == 0)
        fFinalize = (Py_FinalizeFn)LOOKUP_SYMBOL(pylib, "Py_Finalize");
    return fFinalize();
}

void DPy_SetPythonHome(wchar_t *ph)
{
    if (fSetPythonHome == 0)
        fSetPythonHome = (Py_SetPythonHomeFn)LOOKUP_SYMBOL(pylib, "Py_SetPythonHome");
    return fSetPythonHome(ph);
}

void DPy_SetPath(wchar_t *ph)
{
    if (fSetPath == 0)
        fSetPath = (Py_SetPathFn)LOOKUP_SYMBOL(pylib, "Py_SetPath");
    return fSetPath(ph);
}

void DPy_SetProgramName(wchar_t *ph)
{
    if (fSetProgramName == 0)
        fSetProgramName = (Py_SetProgramNameFn)LOOKUP_SYMBOL(pylib, "Py_SetProgramName");
    return fSetProgramName(ph);
}

#endif // defined(DYNAMIC_PYTHON)

