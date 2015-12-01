// Declare the dynamic python versions of the Python API. Handcoded for now.

#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON

#define DECLARE_PY(x) D##x
#define CALL_PY(x) D##x

#else

#define DECLARE_PY(x) D##x
#define CALL_PY(x) x

PyObject* Py_TrueGet() { return Py_True; }
PyObject* Py_FalseGet() { return Py_False; }
PyObject* Py_NoneGet() { return Py_None; }
PyObject* PyExc_GetValueError() { return PyExc_ValueError; }

#endif

int DECLARE_PY(PyCallable_Check)(PyObject *o);
void* DECLARE_PY(PyCapsule_GetPointer)(PyObject *capsule, const char *name);
int DECLARE_PY(PyCapsule_IsValid)(PyObject *capsule, const char *name);
PyObject* DECLARE_PY(PyCapsule_New)(void *pointer, const char *name, PyCapsule_Destructor destructor);
PyObject* DECLARE_PY(PyDict_GetItemString)(PyObject *p, const char *key);
PyObject* DECLARE_PY(PyDict_New)();
int DECLARE_PY(PyDict_SetItem)(PyObject *p, PyObject *key, PyObject *val);
void DECLARE_PY(PyErr_Clear)();
PyObject* DECLARE_PY(PyErr_Format)(PyObject *exception, const char *format, ...);
PyObject* DECLARE_PY(PyErr_Occurred)();
void DECLARE_PY(PyErr_Print)();
void DECLARE_PY(PyErr_SetString)(PyObject *type, const char *message);
PyObject* DECLARE_PY(PyEval_EvalCode)(PyObject *co, PyObject *globals, PyObject *locals);
void DECLARE_PY(PyEval_InitThreads)();
void DECLARE_PY(PyEval_RestoreThread)(PyThreadState *tstate);
PyThreadState* DECLARE_PY(PyEval_SaveThread)();
double DECLARE_PY(PyFloat_AsDouble)(PyObject *o);
PyObject* DECLARE_PY(PyFloat_FromDouble)(double v);
PyGILState_STATE DECLARE_PY(PyGILState_Ensure)();
void DECLARE_PY(PyGILState_Release)(PyGILState_STATE s);
typedef PyObject* (*PyImport_AppendInittabInitFn)(void);
int DECLARE_PY(PyImport_AppendInittab)(const char *name, PyImport_AppendInittabInitFn initfunc);
PyObject* DECLARE_PY(PyImport_GetModuleDict)();
PyObject* DECLARE_PY(PyImport_ImportModule)(const char *name);
int DECLARE_PY(PyList_Append)(PyObject *list, PyObject *item);
PyObject* DECLARE_PY(PyList_GetItem)(PyObject *list, Py_ssize_t index);
int DECLARE_PY(PyList_Insert)(PyObject *list, Py_ssize_t index, PyObject *item);
PyObject* DECLARE_PY(PyList_New)(Py_ssize_t len);
Py_ssize_t DECLARE_PY(PyList_Size)(PyObject *list);
long DECLARE_PY(PyLong_AsLong)(PyObject *obj);
PY_LONG_LONG DECLARE_PY(PyLong_AsLongLong)(PyObject *obj);
PyObject* DECLARE_PY(PyLong_FromLong)(long v);
PyObject* DECLARE_PY(PyLong_FromLongLong)(PY_LONG_LONG v);
PyObject* DECLARE_PY(PyLong_FromUnsignedLong)(unsigned long v);
PyObject* DECLARE_PY(PyLong_FromUnsignedLongLong)(unsigned PY_LONG_LONG v);
int DECLARE_PY(PyMapping_Check)(PyObject *o);
PyObject* DECLARE_PY(PyMapping_Items)(PyObject *o);
int DECLARE_PY(PyModule_AddObject)(PyObject *module, const char *name, PyObject *value);
PyObject* DECLARE_PY(PyModule_Create2)(PyModuleDef *module, int module_api_version);
PyObject* DECLARE_PY(PyModule_GetDict)(PyObject *module);
PyObject* DECLARE_PY(PyObject_CallObject)(PyObject *callable_object, PyObject *args);
PyObject* DECLARE_PY(PyObject_GetAttr)(PyObject *o, PyObject *attr_name);
PyObject* DECLARE_PY(PyObject_GetAttrString)(PyObject *o, const char *attr_name);
int DECLARE_PY(PyObject_IsTrue)(PyObject *o);
int DECLARE_PY(PyObject_SetAttr)(PyObject *o, PyObject *attr_name, PyObject *v);
PyObject* DECLARE_PY(PyRun_SimpleString)(const char *str);
PyObject* DECLARE_PY(PyRun_StringFlags)(const char *str, int start, PyObject *globals, PyObject *locals, PyCompilerFlags *flags);
int DECLARE_PY(PySequence_Check)(PyObject *o);
PyObject* DECLARE_PY(PySequence_Fast)(PyObject *o, const char *m);
Py_ssize_t DECLARE_PY(PySequence_Size)(PyObject *o);
int DECLARE_PY(PyState_AddModule)(PyObject *module, PyModuleDef *def);
PyObject* DECLARE_PY(PyTuple_GetItem)(PyObject *p, Py_ssize_t pos);
PyObject* DECLARE_PY(PyTuple_New)(Py_ssize_t len);
int DECLARE_PY(PyTuple_SetItem)(PyObject *p, Py_ssize_t pos, PyObject *o);
int DECLARE_PY(PyType_IsSubtype)(PyTypeObject *a, PyTypeObject *b);
char* DECLARE_PY(PyUnicode_AsUTF8)(PyObject *unicode);
PyObject* DECLARE_PY(PyUnicode_DecodeUTF16)(const char *s, Py_ssize_t size, const char *errors, int *byteorder);
PyObject* DECLARE_PY(PyUnicode_FromString)(const char *u);
PyObject* DECLARE_PY(Py_CompileStringExFlags)(const char *str, const char *filename, int start, PyCompilerFlags *flags, int optimize);
void DECLARE_PY(Py_Initialize)();
void DECLARE_PY(Py_SetPythonHome)(wchar_t *home);
bool DECLARE_PY(PyBool_Check)(PyObject *o);
bool DECLARE_PY(PyCapsule_CheckExact)(PyObject *o);
bool DECLARE_PY(PyFloat_Check)(PyObject *o);
bool DECLARE_PY(PyModule_Check)(PyObject *o);
PyObject* DECLARE_PY(PyExc_GetAttributeError)();
PyObject* DECLARE_PY(PyExc_GetImportError)();
PyObject* DECLARE_PY(PyExc_GetRuntimeError)();
PyObject* DECLARE_PY(PyExc_GetValueError)();
PyObject* DECLARE_PY(Py_TrueGet)();
PyObject* DECLARE_PY(Py_FalseGet)();
PyObject* DECLARE_PY(Py_NoneGet)();
