// Define the dynamic python methods, if in use.
//
// The Python shared library location is only known at runtime (it may be inside an arbitrary
// venv/conda environment), so these functions cannot be resolved via normal link-time linking.
// Each wrapper below lazily resolves its symbol via LOOKUP_SYMBOL (dlsym/GetProcAddress) the
// first time it is called and caches the resulting function pointer.
//
// Most of the Python C API used here is a simple "look up symbol, cache it, call it" pattern.
// Rather than hand-writing that boilerplate four times per function (typedef, cached pointer,
// wrapper, reset-on-unload), the bulk of the API surface is declared once in the PY_SIMPLE_FUNCTIONS
// table below and the repetitive code is generated from that table with X-macros. A handful of
// functions don't fit that simple shape (type checks against a Python type object, and
// exception/singleton getters that resolve a data symbol rather than a function) and are
// generated from their own small tables.

// for Q_OS defs
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
#define PY_SSIZE_T_CLEAN
#define Py_LIMITED_API 0x030C0000 // Target Python 3.12 and later
#include <Python.h>
#pragma pop_macro("_DEBUG")

#include "PythonStubs.h"

void *pylib = 0;

// Table of the "plain" Python C API functions: PY(return type, name, (parameter list), (argument list)).
// This covers everything that is just "resolve the symbol by name, cache it, call it".
#define PY_SIMPLE_FUNCTIONS(PY) \
    PY(void, PyBuffer_Release, (Py_buffer *o), (o)) \
    PY(int, PyCallable_Check, (PyObject *o), (o)) \
    PY(void*, PyCapsule_GetPointer, (PyObject *capsule, const char *name), (capsule, name)) \
    PY(int, PyCapsule_IsValid, (PyObject *capsule, const char *name), (capsule, name)) \
    PY(PyObject*, PyCapsule_New, (void *pointer, const char *name, PyCapsule_Destructor destructor), (pointer, name, destructor)) \
    PY(PyObject*, PyDict_GetItemString, (PyObject *p, const char *key), (p, key)) \
    PY(PyObject*, PyDict_New, (), ()) \
    PY(int, PyDict_SetItem, (PyObject *p, PyObject *key, PyObject *val), (p, key, val)) \
    PY(void, PyErr_Clear, (), ()) \
    PY(PyObject*, PyErr_Occurred, (), ()) \
    PY(void, PyErr_Print, (), ()) \
    PY(PyObject*, PyErr_NewException, (const char *message, PyObject *base, PyObject *dict), (message, base, dict)) \
    PY(void, PyErr_SetString, (PyObject *type, const char *message), (type, message)) \
    PY(PyObject*, PyEval_EvalCode, (PyObject *co, PyObject *globals, PyObject *locals), (co, globals, locals)) \
    PY(void, PyEval_InitThreads, (), ()) \
    PY(void, PyEval_RestoreThread, (PyThreadState *tstate), (tstate)) \
    PY(PyThreadState*, PyEval_SaveThread, (), ()) \
    PY(double, PyFloat_AsDouble, (PyObject *o), (o)) \
    PY(PyObject*, PyFloat_FromDouble, (double v), (v)) \
    PY(PyGILState_STATE, PyGILState_Ensure, (), ()) \
    PY(int, PyGILState_Check, (), ()) \
    PY(void, PyGILState_Release, (PyGILState_STATE s), (s)) \
    PY(int, PyImport_AppendInittab, (const char *name, PyImport_AppendInittabInitFn initfunc), (name, initfunc)) \
    PY(PyObject*, PyImport_GetModuleDict, (), ()) \
    PY(PyObject*, PyImport_ImportModule, (const char *name), (name)) \
    PY(void, Py_DecRef, (PyObject *o), (o)) \
    PY(void, Py_IncRef, (PyObject *o), (o)) \
    PY(int, PyList_Append, (PyObject *list, PyObject *item), (list, item)) \
    PY(PyObject*, PyList_GetItem, (PyObject *list, Py_ssize_t index), (list, index)) \
    PY(int, PyList_Insert, (PyObject *list, Py_ssize_t index, PyObject *item), (list, index, item)) \
    PY(PyObject*, PyList_New, (Py_ssize_t len), (len)) \
    PY(Py_ssize_t, PyList_Size, (PyObject *list), (list)) \
    PY(long, PyLong_AsLong, (PyObject *obj), (obj)) \
    PY(PyObject*, PyLong_FromLong, (long v), (v)) \
    PY(PyObject*, PyLong_FromLongLong, (PY_LONG_LONG v), (v)) \
    PY(PyObject*, PyLong_FromUnsignedLong, (unsigned long v), (v)) \
    PY(PyObject*, PyLong_FromUnsignedLongLong, (unsigned PY_LONG_LONG v), (v)) \
    PY(int, PyMapping_Check, (PyObject *o), (o)) \
    PY(PyObject*, PyMapping_Items, (PyObject *o), (o)) \
    PY(int, PyModule_AddObject, (PyObject *module, const char *name, PyObject *value), (module, name, value)) \
    PY(PyObject*, PyModule_Create2, (PyModuleDef *module, int module_api_version), (module, module_api_version)) \
    PY(PyObject*, PyModule_GetDict, (PyObject *module), (module)) \
    PY(PyObject*, PyObject_CallObject, (PyObject *callable_object, PyObject *args), (callable_object, args)) \
    PY(PyObject*, PyObject_GetAttr, (PyObject *o, PyObject *attr_name), (o, attr_name)) \
    PY(PyObject*, PyObject_GetAttrString, (PyObject *o, const char *attr_name), (o, attr_name)) \
    PY(int, PyObject_GetBuffer, (PyObject *o, Py_buffer *view, int flags), (o, view, flags)) \
    PY(int, PyObject_HasAttrString, (PyObject *o, const char *attr_name), (o, attr_name)) \
    PY(int, PyObject_IsTrue, (PyObject *o), (o)) \
    PY(int, PyObject_SetAttr, (PyObject *o, PyObject *attr_name, PyObject *v), (o, attr_name, v)) \
    PY(PyObject*, PyObject_Type, (PyObject *o), (o)) \
    PY(PyObject*, PyRun_SimpleString, (const char *str), (str)) \
    PY(int, PySequence_Check, (PyObject *o), (o)) \
    PY(PyObject*, PySequence_GetItem, (PyObject *o, Py_ssize_t i), (o, i)) \
    PY(Py_ssize_t, PySequence_Size, (PyObject *o), (o)) \
    PY(int, PyState_AddModule, (PyObject *module, PyModuleDef *def), (module, def)) \
    PY(PyObject*, PyTuple_GetItem, (PyObject *p, Py_ssize_t pos), (p, pos)) \
    PY(PyObject*, PyTuple_New, (Py_ssize_t len), (len)) \
    PY(int, PyTuple_SetItem, (PyObject *p, Py_ssize_t pos, PyObject *o), (p, pos, o)) \
    PY(int, PyType_IsSubtype, (PyTypeObject *a, PyTypeObject *b), (a, b)) \
    PY(char*, PyUnicode_AsUTF8, (PyObject *unicode), (unicode)) \
    PY(PyObject*, PyUnicode_DecodeUTF16, (const char *s, Py_ssize_t size, const char *errors, int *byteorder), (s, size, errors, byteorder)) \
    PY(PyObject*, PyUnicode_FromString, (const char *u), (u)) \
    PY(wchar_t*, PyUnicode_AsWideCharString, (PyObject *unicode, Py_ssize_t *size), (unicode, size)) \
    PY(void, PyMem_Free, (void *p), (p)) \
    PY(void, Py_Initialize, (), ()) \
    PY(void, Py_Finalize, (), ()) \
    PY(void, Py_SetPythonHome, (wchar_t *home), (home)) \
    PY(void, Py_SetPath, (wchar_t *home), (home)) \
    PY(void, Py_SetProgramName, (wchar_t *home), (home))

#define PY_DECLARE_TYPEDEF(RET, NAME, PARAMS, ARGS) typedef RET (*NAME##Fn) PARAMS;
PY_SIMPLE_FUNCTIONS(PY_DECLARE_TYPEDEF)
#undef PY_DECLARE_TYPEDEF

#define PY_DECLARE_CACHE(RET, NAME, PARAMS, ARGS) static NAME##Fn f##NAME = 0;
PY_SIMPLE_FUNCTIONS(PY_DECLARE_CACHE)
#undef PY_DECLARE_CACHE

#define PY_DEFINE_WRAPPER(RET, NAME, PARAMS, ARGS) \
    RET D##NAME PARAMS \
    { \
        if (f##NAME == 0) \
            f##NAME = (NAME##Fn)LOOKUP_SYMBOL(pylib, #NAME); \
        return f##NAME ARGS; \
    }
PY_SIMPLE_FUNCTIONS(PY_DEFINE_WRAPPER)
#undef PY_DEFINE_WRAPPER

// Type checks that need to look up a Python type object (a data symbol, not a function) and test
// the argument's type against it. PY_SUBTYPE_CHECKS compares via PyType_IsSubtype (matches
// subclasses too); PY_EXACT_CHECKS compares the type pointer directly.
#define PY_SUBTYPE_CHECKS(PY) \
    PY(bool, PyBool_Check, PyBool_Type) \
    PY(int, PyDict_Check, PyDict_Type) \
    PY(bool, PyFloat_Check, PyFloat_Type) \
    PY(int, PyList_Check, PyList_Type) \
    PY(bool, PyLong_Check, PyLong_Type) \
    PY(int, PyTuple_Check, PyTuple_Type) \
    PY(bool, PyUnicode_Check, PyUnicode_Type)

#define PY_EXACT_CHECKS(PY) \
    PY(bool, PyCapsule_CheckExact, PyCapsule_Type) \
    PY(bool, PyModule_Check, PyModule_Type)

#define PY_DEFINE_SUBTYPE_CHECK(RET, NAME, TYPE_SYMBOL) \
    RET D##NAME(PyObject *o) \
    { \
        PyTypeObject *type_obj = (PyTypeObject *)LOOKUP_SYMBOL(pylib, #TYPE_SYMBOL); \
        PyTypeObject *obj_type = (PyTypeObject *)DPyObject_Type(o); \
        auto is_subtype = CALL_PY(PyType_IsSubtype)(obj_type, type_obj); \
        CALL_PY(Py_DecRef)((PyObject *)obj_type); \
        return is_subtype; \
    }
PY_SUBTYPE_CHECKS(PY_DEFINE_SUBTYPE_CHECK)
#undef PY_DEFINE_SUBTYPE_CHECK

#define PY_DEFINE_EXACT_CHECK(RET, NAME, TYPE_SYMBOL) \
    RET D##NAME(PyObject *o) \
    { \
        PyTypeObject *type_obj = (PyTypeObject *)LOOKUP_SYMBOL(pylib, #TYPE_SYMBOL); \
        return Py_TYPE(o) == type_obj; \
    }
PY_EXACT_CHECKS(PY_DEFINE_EXACT_CHECK)
#undef PY_DEFINE_EXACT_CHECK

// Exception objects and singletons are exposed as data symbols rather than functions, so these
// getters resolve the symbol and return it directly (matching the previous, uncached behavior).
#define PY_GLOBAL_GETTERS(PY) \
    PY(PyExc_GetAttributeError, PyExc_AttributeError) \
    PY(PyExc_GetImportError, PyExc_ImportError) \
    PY(PyExc_GetRuntimeError, PyExc_RuntimeError) \
    PY(PyExc_GetValueError, PyExc_ValueError) \
    PY(Py_TrueGet, _Py_TrueStruct) \
    PY(Py_FalseGet, _Py_FalseStruct) \
    PY(Py_NoneGet, _Py_NoneStruct)

#define PY_DEFINE_GLOBAL_GETTER(NAME, SYMBOL) \
    PyObject *D##NAME() \
    { \
        return (PyObject *)LOOKUP_SYMBOL(pylib, #SYMBOL); \
    }
PY_GLOBAL_GETTERS(PY_DEFINE_GLOBAL_GETTER)
#undef PY_DEFINE_GLOBAL_GETTER

// PyErr_Format is variadic and cannot be forwarded through a generic wrapper; it is unused
// elsewhere in this project, so it is left as a resolve-only stub (matching prior behavior).
typedef PyObject* (*PyErr_FormatFn)(PyObject *exception, const char *format, ...);
static PyErr_FormatFn fErr_Format = 0;
PyObject* DPyErr_Format(PyObject *exception, const char *format, ...)
{
    if (fErr_Format == 0)
        fErr_Format = (PyErr_FormatFn)LOOKUP_SYMBOL(pylib, "PyErr_Format");
    return 0;
}

void initialize_pylib(void *dl)
{
    pylib = dl;
}

void deinitialize_pylib()
{
    pylib = NULL;

#define PY_RESET_CACHE(RET, NAME, PARAMS, ARGS) f##NAME = 0;
    PY_SIMPLE_FUNCTIONS(PY_RESET_CACHE)
#undef PY_RESET_CACHE
    fErr_Format = 0;
}
