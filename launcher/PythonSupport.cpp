/*
 Copyright (c) 2012-2024 Bruker, Inc.
*/

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdint.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define OS_WINDOWS 1
#else
#define OS_WINDOWS 0
#endif

#if defined(__APPLE__)
#define OS_MACOS 1
#else
#define OS_MACOS 0
#endif

#if defined(__linux__)
#define OS_LINUX 1
#else
#define OS_LINUX 0
#endif

#if !OS_WINDOWS
#include <dlfcn.h>
#define LOOKUP_SYMBOL dlsym
#else
#define LOOKUP_SYMBOL GetProcAddress
#endif

// monotonic clock (nanoseconds), defined in DocumentWindow.cpp -- used below to time the
// GIL-wait vs. call-body portions of invokePyMethod when the optional out-params are supplied.
extern int64_t GetCurrentTime();

#include "PythonSupport.h"
#include "PythonStubs.h"

#include "Image.h"
#include "FileSystem.h"

#if OS_WINDOWS
#include <Windows.h>
#include <WinBase.h>
// <Windows.h> (via WinBase.h) #defines GetCurrentTime as GetTickCount for legacy MFC
// compatibility, which would otherwise silently replace this file's uses of the monotonic-
// nanoseconds GetCurrentTime() (declared above, defined in DocumentWindow.cpp) with the coarse
// (~15ms resolution) Win32 GetTickCount -- see the identical guard/comment in DocumentWindow.cpp.
// Without this, gil_wait_ns/body_ns in invokePyMethod below would silently measure near-zero
// deltas for any call shorter than a tick, rather than a real error.
#undef GetCurrentTime
#endif

static PythonSupport *thePythonSupport = NULL;
const char* PythonSupport::qobject_capsule_name = "b93c9a511d32.qobject";

std::string PythonSupport::ensurePython(FileSystem *fs, const std::string &python_home)
{
    if (!python_home.empty() && fs->exists(python_home))
    {
        std::cout << "Using Python environment: " << python_home << std::endl;
        return python_home;
    }
    return std::string();
}

void PythonSupport::initInstance(FileSystem *fs, const std::string &python_home, const std::string &python_library)
{
    thePythonSupport = new PythonSupport(fs, python_home, python_library);
}

void PythonSupport::deinitInstance()
{
    delete thePythonSupport;
    thePythonSupport = NULL;
}

PythonSupport *PythonSupport::instance()
{
    return thePythonSupport;
}

class PlatformSupport
{
public:
    virtual ~PlatformSupport();
    virtual void loadLibrary(FileSystem *fs, const std::string &python_home, const std::string &filePath) = 0;
    virtual void *lookupSymbol(const std::string &symbolName) = 0;
    virtual bool isValid() const = 0;
    virtual void buildVirtualEnvironmentPaths(FileSystem *fs, const std::string &python_home, const std::string &home_bin_path, const std::string &version, std::list<std::string> &filePaths) = 0;
    virtual void buildStandardPaths(FileSystem *fs, const std::string &python_home, std::list<std::string> &filePaths) = 0;
    virtual std::string findLandmarkLibrary(FileSystem *fs, const std::string &filePath) = 0;
    virtual std::string findProgramName(FileSystem *fs, const std::string &python_home) = 0;
    virtual std::string joinedPathSeparator() = 0;
    virtual void buildLibraryPaths(FileSystem *fs, const std::string &python_home, const std::string &python_home_new, std::list<std::string> &filePaths) = 0;
};

PlatformSupport::~PlatformSupport()
{
}

// The platform-specific library search below used to hard-code every supported Python minor
// version (3.12, 3.13, 3.14, ...) in near-identical lists that needed an edit every time a new
// Python version shipped. Instead, each platform searches for a single glob pattern (e.g.
// "libpython3.*.so") and the helpers below pick out the newest matching version, so new Python
// releases are picked up automatically.

// Extracts the Python minor version embedded in a candidate library/DLL file name, e.g.
// "libpython3.14.dylib", "libpython3.14.so", and "Python314.dll" all yield 14. Returns -1 if no
// version number could be found.
static int pythonMinorVersion(const std::string &filePath)
{
    std::string lower = filePath;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    const std::string marker = "python3";
    size_t pos = lower.rfind(marker);
    if (pos == std::string::npos)
        return -1;
    pos += marker.size();
    if (pos < lower.size() && lower[pos] == '.')
        ++pos;
    std::string digits;
    while (pos < lower.size() && std::isdigit(static_cast<unsigned char>(lower[pos])))
        digits += lower[pos++];
    return digits.empty() ? -1 : std::stoi(digits);
}

// Lowest Python minor version this build supports, matching the Py_LIMITED_API target
// (0x030C0000) in PythonSupport.h. Candidates for older interpreters are ignored so an
// unsupported system Python is never picked up just because its name also matched the glob.
static const int kMinSupportedPythonMinorVersion = 12;

// Searches directoryPath (and its subdirectories) for files matching namePattern (a glob such as
// "libpython3.*.so", with '*' standing in for the minor version) and appends any matches to
// filePaths, skipping versions older than kMinSupportedPythonMinorVersion. If
// requiredParentDirectoryName is non-empty, only matches whose immediate parent directory has
// that name are kept, to avoid picking up unrelated nested copies of the library.
static void collectVersionedFiles(FileSystem *fs, const std::string &directoryPath, const std::string &namePattern, std::list<std::string> &filePaths, const std::string &requiredParentDirectoryName = std::string())
{
    std::list<std::string> matches;
    fs->iterateDirectory(directoryPath, std::list<std::string>{ namePattern }, matches);
    for (auto &match : matches)
        if (pythonMinorVersion(match) >= kMinSupportedPythonMinorVersion)
            if (requiredParentDirectoryName.empty() || fs->directoryName(match) == requiredParentDirectoryName)
                filePaths.push_back(match);
}

// Sorts candidate file paths so the newest Python version is tried first, while otherwise
// preserving relative order (e.g. the priority of the directory a candidate was found in), since
// std::list::sort is guaranteed stable.
static void sortNewestVersionFirst(std::list<std::string> &filePaths)
{
    filePaths.sort([](const std::string &a, const std::string &b) {
        return pythonMinorVersion(a) > pythonMinorVersion(b);
    });
}

#if OS_MACOS
class MacSupport : public PlatformSupport
{
    void *dl;
public:
    MacSupport() : dl(nullptr) { }
    ~MacSupport()
    {
        extern void deinitialize_pylib();
        deinitialize_pylib();
        if (dl != nullptr)
            dlclose(dl);
        dl = nullptr;
    }

    virtual void loadLibrary(FileSystem *fs, const std::string &python_home, const std::string &filePath) override
    {
        dl = dlopen(filePath.c_str(), RTLD_LAZY);
        extern void initialize_pylib(void *);
        initialize_pylib(dl);
    }

    virtual void *lookupSymbol(const std::string &symbolName) override
    {
        return dlsym(dl, symbolName.c_str());
    }

    virtual bool isValid() const override
    {
        return dl != nullptr;
    }

    virtual void buildVirtualEnvironmentPaths(FileSystem *fs, const std::string &python_home, const std::string &home_bin_path, const std::string &version, std::list<std::string> &filePaths) override
    {
        std::list<std::string> directories;
        directories.push_back(fs->absoluteFilePath(home_bin_path, "../lib"));
        directories.push_back(fs->absoluteFilePath(home_bin_path, "/usr/local/Cellar/python@" + version));

        for (auto directory : directories)
            collectVersionedFiles(fs, directory, "libpython3.*.dylib", filePaths, "lib");

        sortNewestVersionFirst(filePaths);
    }

    virtual void buildStandardPaths(FileSystem *fs, const std::string &python_home, std::list<std::string> &filePaths) override
    {
        collectVersionedFiles(fs, fs->absoluteFilePath(python_home, "lib"), "libpython3.*.dylib", filePaths, "lib");
        sortNewestVersionFirst(filePaths);
    }

    virtual void buildLibraryPaths(FileSystem *fs, const std::string &python_home, const std::string &python_home_new, std::list<std::string> &filePaths) override
    {
    }

    virtual std::string findLandmarkLibrary(FileSystem *fs, const std::string &filePath) override
    {
        return fs->parentDirectory(filePath);
    }

    virtual std::string findProgramName(FileSystem *fs, const std::string &python_home) override
    {
        return fs->absoluteFilePath(python_home, "bin/python3");
    }

    virtual std::string joinedPathSeparator() override
    {
        return ":";
    }
};
#endif

#if OS_LINUX
class LinuxSupport : public PlatformSupport
{
    void *dl;
public:
    LinuxSupport() : dl(nullptr) { }
    ~LinuxSupport()
    {
        extern void deinitialize_pylib();
        deinitialize_pylib();
        if (dl != nullptr)
            dlclose(dl);
        dl = nullptr;
    }

    virtual void loadLibrary(FileSystem *fs, const std::string &python_home, const std::string &filePath) override
    {
        dl = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        extern void initialize_pylib(void *);
        initialize_pylib(dl);
    }

    virtual void *lookupSymbol(const std::string &symbolName) override
    {
        return dlsym(dl, symbolName.c_str());
    }

    virtual bool isValid() const override
    {
        return dl != nullptr;
    }

    virtual void buildVirtualEnvironmentPaths(FileSystem *fs, const std::string &python_home, const std::string &home_bin_path, const std::string &version, std::list<std::string> &filePaths) override
    {
        std::string homeParentDirectory = fs->parentDirectory(home_bin_path);
        collectVersionedFiles(fs, fs->absoluteFilePath(homeParentDirectory, "lib"), "libpython3.*.so", filePaths);
        sortNewestVersionFirst(filePaths);
    }

    virtual void buildStandardPaths(FileSystem *fs, const std::string &python_home, std::list<std::string> &filePaths) override
    {
        collectVersionedFiles(fs, fs->absoluteFilePath(python_home, "lib"), "libpython3.*.so", filePaths);
        sortNewestVersionFirst(filePaths);
    }

    virtual void buildLibraryPaths(FileSystem *fs, const std::string &python_home, const std::string &python_home_new, std::list<std::string> &filePaths) override
    {
    }

    virtual std::string findLandmarkLibrary(FileSystem *fs, const std::string &filePath) override
    {
        std::string directory = fs->directory(filePath);
        std::string lastDirectory;
        while (directory != lastDirectory && fs->directoryName(directory) != "lib")
        {
            lastDirectory = directory;
            directory = fs->parentDirectory(directory);
        }
        return fs->parentDirectory(directory);
    }

    virtual std::string findProgramName(FileSystem *fs, const std::string &python_home) override
    {
        return fs->absoluteFilePath(python_home, "bin/python3");
    }

    virtual std::string joinedPathSeparator() override
    {
        return ":";
    }
};
#endif

#if OS_WINDOWS
class WinSupport : public PlatformSupport
{
    void *dl;
public:
    WinSupport() : dl(nullptr) { }
    ~WinSupport()
    {
        extern void deinitialize_pylib();
        deinitialize_pylib();
        if (dl != nullptr)
            FreeLibrary((HMODULE)dl);
        dl = nullptr;
    }

    virtual void loadLibrary(FileSystem *fs, const std::string &python_home, const std::string &filePath) override
    {
        // Python may have side-by-side DLLs that it uses. This seems to be an issue with how
        // Anaconda handles installation of the VS redist -- they include it in the directory
        // rather than installing it system wide. That approach works great when running the
        // python.exe, but not so great when loading the python35.dll. To avoid _us_ having to
        // install the VS redist, we add the python home to the DLL search path. Through trial
        // and error, the qputenv or SetDllDirectory approach works. The AddDllDirectory does not
        // work. I leave this code here so the next time someone encounters it, they can try these
        // different solutions.

        // DOES NOT WORK
        //SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_USER_DIRS);
        //AddDllDirectory((PCWSTR)QDir::toNativeSeparators(python_home).utf16());  // ensure that DLLs local to Python can be found

        // DOES NOT WORK
        //QProcessEnvironment::systemEnvironment().insert("PATH", QProcessEnvironment::systemEnvironment().value("PATH") + ";" + python_home);

        // WORKS. Library/bin added for Anaconda/Miniconda 3.7 compatibility.
        fs->putEnv("PATH", (fs->getEnv("PATH") + ";" + python_home + ";" + fs->absoluteFilePath(python_home, "Library/bin")));

        // WORKS ALMOST. DOESN'T ALLOW CAMERA PLUG-INS TO LOAD.
        //SetDllDirectory(QDir::toNativeSeparators(python_home).toUtf8());  // ensure that DLLs local to Python can be found

        // required, see https://bugs.python.org/issue36085
        std::string dll_directory = fs->toNativeSeparators(fs->absoluteFilePath(python_home, "Library/bin"));
        std::wstring dll_directory_w = std::wstring(dll_directory.begin(), dll_directory.end());
        AddDllDirectory(dll_directory_w.c_str());

        dl = LoadLibraryA(fs->toNativeSeparators(filePath).c_str());

        extern void initialize_pylib(void *);
        initialize_pylib(dl);
    }

    virtual void *lookupSymbol(const std::string &symbolName) override
    {
        return GetProcAddress(HMODULE(dl), symbolName.c_str());
    }

    virtual bool isValid() const override
    {
        return dl != nullptr;
    }

    virtual void buildVirtualEnvironmentPaths(FileSystem *fs, const std::string &python_home, const std::string &home_bin_path, const std::string &version, std::list<std::string> &filePaths) override
    {
        std::list<std::string> directories;
        directories.push_back(fs->absoluteFilePath(python_home, "Scripts"));
        directories.push_back(python_home);
        directories.push_back(fs->absoluteFilePath(home_bin_path, "Scripts"));
        directories.push_back(home_bin_path);

        for (auto directory : directories)
            collectVersionedFiles(fs, directory, "Python3??.dll", filePaths);

        sortNewestVersionFirst(filePaths);
    }

    virtual void buildStandardPaths(FileSystem *fs, const std::string &python_home, std::list<std::string> &filePaths) override
    {
        collectVersionedFiles(fs, python_home, "Python3??.dll", filePaths);
        sortNewestVersionFirst(filePaths);
    }

    virtual void buildLibraryPaths(FileSystem *fs, const std::string &python_home, const std::string &python_home_new, std::list<std::string> &filePaths) override
    {
        collectVersionedFiles(fs, fs->absoluteFilePath(python_home, "Scripts"), "python3??.zip", filePaths);
        sortNewestVersionFirst(filePaths);
        filePaths.push_back(fs->absoluteFilePath(python_home_new, "DLLs"));
        filePaths.push_back(fs->absoluteFilePath(python_home_new, "lib"));
        filePaths.push_back(python_home_new);
        filePaths.push_back(python_home);
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/site-packages"));
    }

    virtual std::string findLandmarkLibrary(FileSystem *fs, const std::string &filePath) override
    {
        return fs->directory(filePath);
    }

    virtual std::string findProgramName(FileSystem *fs, const std::string &python_home) override
    {
        return fs->absoluteFilePath(python_home, "Python.exe");
    }

    virtual std::string joinedPathSeparator() override
    {
        return ";";
    }
};
#endif

PythonSupport::PythonSupport(FileSystem *fs_, const std::string &python_home, const std::string &python_library)
    : module_exception(NULL)
    , fs(fs_)
{
#if OS_MACOS
    ps = std::unique_ptr<PlatformSupport>(new MacSupport());
#elif OS_LINUX
    ps = std::unique_ptr<PlatformSupport>(new LinuxSupport());
#else
    ps = std::unique_ptr<PlatformSupport>(new WinSupport());
#endif
    std::list<std::string> filePaths;
    std::string venv_conf_file_name = fs->absoluteFilePath(python_home, "pyvenv.cfg");
    if (!python_library.empty())
    {
        filePaths.push_back(python_library);
    }
    else if (fs->exists(venv_conf_file_name))
    {
        std::string home_bin_path;
        std::string version;
        if (fs->parseConfigFile(venv_conf_file_name, home_bin_path, version))
            ps->buildVirtualEnvironmentPaths(fs.get(), python_home, home_bin_path, version, filePaths);
    }
    else
    {
        // probably conda or standard Python
        ps->buildStandardPaths(fs.get(), python_home, filePaths);
    }

    std::string filePath;
    for (auto filePath_ : filePaths)
    {
        if (fs->exists(filePath_))
        {
            filePath = filePath_;
            break;
        }
    }

    m_actual_python_home = ps->findLandmarkLibrary(fs.get(), filePath);

    ps->loadLibrary(fs.get(), python_home, filePath);

    m_valid = ps->isValid();

    dynamic_PyArg_ParseTuple = (PyArg_ParseTupleFn)ps->lookupSymbol("PyArg_ParseTuple");
    dynamic_Py_BuildValue = (Py_BuildValueFn)ps->lookupSymbol("Py_BuildValue");
}

PythonSupport::PythonSupport(PythonSupport const &)
{
}

PythonSupport& PythonSupport::operator=(PythonSupport const &)
{
    return *this;
}

PythonSupport::~PythonSupport()
{
    PyXDecRef(module_exception);
}

struct Python_ThreadBlockState
{
    PyGILState_STATE gstate;
};

void Python_ThreadBlock::release()
{
    if (m_state)
    {
        CALL_PY(PyGILState_Release)(m_state->gstate);
        delete m_state;
        m_state = NULL;
    }
}

void Python_ThreadBlock::grab()
{
    m_state = new Python_ThreadBlockState();
    m_state->gstate = CALL_PY(PyGILState_Ensure)();
}

struct Python_ThreadAllowState
{
    PyThreadState *gstate;
};

void Python_ThreadAllow::release()
{
    if (m_state)
    {
        CALL_PY(PyEval_RestoreThread)(m_state->gstate);
        delete m_state;
        m_state = NULL;
    }
}

void Python_ThreadAllow::grab()
{
    m_state = new Python_ThreadAllowState();
    m_state->gstate = CALL_PY(PyEval_SaveThread)();
}

std::string join(std::list<std::string>::const_iterator begin, std::list<std::string>::const_iterator end, const std::string &separator)
{
    std::string joined;
    bool first = true;
    for (auto iter = begin; iter != end; ++iter, first = false)
    {
        if (!first)
            joined += separator + *iter;
        else
            joined = *iter;
    }
    return joined;
}

static std::wstring python_home_static;
static std::wstring python_program_name_static;

void PythonSupport::initialize(const std::string &python_home, const std::list<std::string> &python_paths, const std::string &python_library)
{
    auto python_paths_ = python_paths;

    std::string python_home_new = python_home;
    std::string python_program_name = ps->findProgramName(fs.get(), python_home);

    // check if we're running inside a venv, determined by whether pyvenv.cfg exists.
    // if so, read the config file and find the home key, indicating the python installation
    // directory (with /bin attached). then call SetPythonHoCme and SetProgramName with the
    // installation directory and virtual environment python path respectively. these are
    // required for the virtual environment to load correctly.
    std::string venv_conf_file_name = fs->absoluteFilePath(python_home, "pyvenv.cfg");
    if (fs->exists(venv_conf_file_name))
    {
        python_home_new = m_actual_python_home;

        // may be required to configure the path; see https://bugs.python.org/issue34725
        ps->buildLibraryPaths(fs.get(), python_home, python_home_new, python_paths_);
    }

    if (!python_paths_.empty())
    {
        std::string path = join(python_paths_.begin(), python_paths_.end(), ps->joinedPathSeparator());
        std::wstring path_w(path.begin(), path.end());
        CALL_PY(Py_SetPath)(path_w.data());
    }

    python_home_static = std::wstring(python_home_new.begin(), python_home_new.end());
    CALL_PY(Py_SetPythonHome)(python_home_static.data());  // requires a permanent buffer

    python_program_name_static = std::wstring(python_program_name.begin(), python_program_name.end());
    CALL_PY(Py_SetProgramName)(python_program_name_static.data());  // requires a permanent buffer

    CALL_PY(Py_Initialize)();

    // release the GIL. the tool will normally run with the GIL released. calls back to Python
    // will need to acquire the GIL. the initial state is saved because the GIL is required to
    // finalize.
    m_initial_state = CALL_PY(PyEval_SaveThread)();
}

void PythonSupport::deinitialize()
{
    // grab the GIL that was released after Py_Initialize.
    CALL_PY(PyEval_RestoreThread)(m_initial_state);

    // finalize.
    CALL_PY(Py_Finalize)();
}

void *UnwrapObject2(PyObject *py_obj)
{
    void *ptr = NULL;
    if (CALL_PY(PyCapsule_CheckExact)(py_obj))
        ptr = static_cast<void *>(CALL_PY(PyCapsule_GetPointer)(py_obj, PythonSupport::qobject_capsule_name));
    return ptr;
}

void *PythonSupport::UnwrapObject(PyObject *py_obj)
{
    return UnwrapObject2(py_obj);
}

PyObject *PythonValueVariantToPyObject(const PythonValueVariant &value_variant)
{
    if (value_variant.value.valueless_by_exception())
    {
        PyIncRef(CALL_PY(Py_NoneGet)());
        return CALL_PY(Py_NoneGet)();
    }
    else if (std::holds_alternative<bool>(value_variant.value))
    {
        PyObject *py_obj = *std::get_if<bool>(&value_variant.value) ? CALL_PY(Py_TrueGet)() : CALL_PY(Py_FalseGet)();
        PyIncRef(py_obj);
        return py_obj;
    }
    else if (std::holds_alternative<long>(value_variant.value))
    {
        return CALL_PY(PyLong_FromLong)(*std::get_if<long>(&value_variant.value));
    }
    else if (std::holds_alternative<long long>(value_variant.value))
    {
        return CALL_PY(PyLong_FromLongLong)(*std::get_if<long long>(&value_variant.value));
    }
    else if (std::holds_alternative<double>(value_variant.value))
    {
        return CALL_PY(PyFloat_FromDouble)(*std::get_if<double>(&value_variant.value));
    }
    else if (std::holds_alternative<void *>(value_variant.value))
    {
        return CALL_PY(PyCapsule_New)(*std::get_if<void *>(&value_variant.value), PythonSupport::qobject_capsule_name, NULL);
    }
    else if (std::holds_alternative<std::string>(value_variant.value))
    {
        return CALL_PY(PyUnicode_FromString)(std::get_if<std::string>(&value_variant.value)->c_str());
    }
    else if (std::holds_alternative<std::map<std::string, PythonValueVariant>>(value_variant.value))
    {
        PyObject *py_map = CALL_PY(PyDict_New)();
        for (auto item: *std::get_if<std::map<std::string, PythonValueVariant>>(&value_variant.value))
        {
            PyObject *py_key = CALL_PY(PyUnicode_FromString)(item.first.c_str());
            PyObject *py_value = PythonValueVariantToPyObject(item.second);
            CALL_PY(PyDict_SetItem)(py_map, py_key, py_value);
            PyDecRef(py_key);
            PyDecRef(py_value);
        }
        return py_map;
    }
    else if (std::holds_alternative<std::vector<PythonValueVariant>>(value_variant.value))
    {
        auto variant_list = std::get_if<std::vector<PythonValueVariant>>(&value_variant.value);
        PyObject *py_list = CALL_PY(PyTuple_New)(variant_list->size());
        int i = 0;
        for (auto item: *variant_list)
        {
            CALL_PY(PyTuple_SetItem)(py_list, i, PythonValueVariantToPyObject(item)); // steals reference
            i++;
        }
        return py_list;
    }
    else if (std::holds_alternative<PyObjectPtr>(value_variant.value))
    {
        const PyObjectPtr *ptr = std::get_if<PyObjectPtr>(&value_variant.value);
        PyObject *py_object = ptr->get();
        PyIncRef(py_object);
        return py_object;
    }

    PyIncRef(CALL_PY(Py_NoneGet)());
    return CALL_PY(Py_NoneGet)();
}

PythonValueVariant PyObjectToValueVariant(PyObject *py_object)
{
    if (CALL_PY(PyUnicode_Check)(py_object))
    {
        return PythonValueVariant{std::string(CALL_PY(PyUnicode_AsUTF8)(py_object))};
    }
    else if (CALL_PY(PyBool_Check)(py_object))
    {
        return PythonValueVariant{static_cast<bool>(CALL_PY(PyObject_IsTrue)(py_object))};
    }
    else if (CALL_PY(PyLong_Check)(py_object))
    {
        return PythonValueVariant{PyInt_AsLong(py_object)};
    }
    else if (CALL_PY(PyFloat_Check)(py_object))
    {
        return PythonValueVariant{CALL_PY(PyFloat_AsDouble)(py_object)};
    }
    else if (CALL_PY(PyCapsule_IsValid)(py_object, PythonSupport::qobject_capsule_name) && CALL_PY(PyCapsule_CheckExact)(py_object))
    {
        return PythonValueVariant{CALL_PY(PyCapsule_GetPointer)(py_object, PythonSupport::qobject_capsule_name)};
    }
    else if (CALL_PY(PyDict_Check)(py_object))
    {
        std::map<std::string, PythonValueVariant> map;
        PyObject *items = CALL_PY(PyMapping_Items)(py_object);
        if (items)
        {
            int count = (int)CALL_PY(PyList_Size)(items);
            for (int i=0; i<count; i++)
            {
                PyObject *tuple = CALL_PY(PyList_GetItem)(items,i); //borrowed
                PyObject *key = CALL_PY(PyTuple_GetItem)(tuple, 0); //borrowed
                PyObject *value = CALL_PY(PyTuple_GetItem)(tuple, 1); //borrowed
                std::string key_string = std::string(CALL_PY(PyUnicode_AsUTF8)(key));
                PythonValueVariant value_variant = PyObjectToValueVariant(value);
                map.insert(std::pair(key_string, value_variant));
            }
            PyDecRef(items);
        }
        return PythonValueVariant{map};
    }
    else if (((CALL_PY(PyList_Check)(py_object) || CALL_PY(PyTuple_Check)(py_object))) && CALL_PY(PySequence_Check)(py_object))
    {
        std::vector<PythonValueVariant> list;
        int count = (int)CALL_PY(PySequence_Size)(py_object);
        for (int i=0; i<count; i++)
        {
            auto item = CALL_PY(PySequence_GetItem(py_object, i));
            list.push_back(PyObjectToValueVariant(item));
            PyDecRef(item);
        }
        return PythonValueVariant{list};
    }
    else if (py_object == CALL_PY(Py_NoneGet)())
    {
        return PythonValueVariant();
    }
    else
    {
        PyObjectPtr py_object_ptr;
        py_object_ptr.setPyObject(py_object);
        return PythonValueVariant{py_object_ptr};
    }
}

void PythonSupport::addResourcePath(const std::string &resources_path)
{
    PyObject *sys_module = CALL_PY(PyImport_ImportModule)("sys");
    PyObject *py_path = CALL_PY(PyObject_GetAttrString)(sys_module, "path");
    PyObject *py_filename = CALL_PY(PyUnicode_FromString)(resources_path.c_str());
    CALL_PY(PyList_Insert)(py_path, 1, py_filename);
    PyDecRef(py_filename);
    PyDecRef(py_path);
    PyDecRef(sys_module);
}

PythonValueVariant PythonSupport::invokePyMethod(PyObjectPtr *object, const std::string &method, const std::list<PythonValueVariant> &args, int64_t *gil_wait_ns, int64_t *body_ns)
{
    // see header comment: only pay for the timestamps when a caller actually wants them.
    int64_t wait_start_ns = (gil_wait_ns || body_ns) ? GetCurrentTime() : 0;
    Python_ThreadBlock thread_block;
    int64_t gil_acquired_ns = (gil_wait_ns || body_ns) ? GetCurrentTime() : 0;
    if (gil_wait_ns)
        *gil_wait_ns = gil_acquired_ns - wait_start_ns;

    // note: PythonValueVariant is not copy/move-assignable (see std::variant<..., PyObjectPtr>
    // member), so record body_ns via a small helper that runs just before each return below
    // rather than assigning through a single local result variable.
    auto mark_body_end = [&]() { if (body_ns) *body_ns = GetCurrentTime() - gil_acquired_ns; };

    PyObject *py_object = object->get();

    if (py_object)
    {
        PyObjectPtr callable(CALL_PY(PyObject_GetAttrString)(py_object, method.c_str()));
        if (CALL_PY(PyCallable_Check)(callable))
        {
            CALL_PY(PyErr_Clear)();

            bool err = false;

            PyObjectPtr py_args(args.size() > 0 ? CALL_PY(PyTuple_New)(args.size()) : NULL);

            int index = 0;
            for (auto arg: args)
            {
                PyObject *obj = PythonValueVariantToPyObject(arg);
                if (obj)
                {
                    // steals reference
                    CALL_PY(PyTuple_SetItem)(py_args, index, obj);
                }
                else
                {
                    err = true;
                    break;
                }
                ++index;
            }

            if (!err)
            {
                CALL_PY(PyErr_Clear)();
                PyObjectPtr py_result(CALL_PY(PyObject_CallObject)(callable, py_args));
                if (py_result)
                {
                    mark_body_end();
                    return PyObjectToValueVariant(py_result);
                }
                else
                {
                    CALL_PY(PyErr_Print)();
                    CALL_PY(PyErr_Clear)();
                }
            }
        }
    }

    mark_body_end();

    return PythonValueVariant();
}


PythonValueVariant PythonSupport::getAttribute(PyObjectPtr *object, const std::string &attribute)
{
    Python_ThreadBlock thread_block;

    PyObject *py_object = object->get();

    if (py_object)
    {
        PyObjectPtr py_attribute(CALL_PY(PyUnicode_FromString)(attribute.c_str()));
        if (py_attribute)
        {
            CALL_PY(PyErr_Clear)();
            PyObjectPtr py_result(CALL_PY(PyObject_GetAttr)(py_object, py_attribute));
            if (py_result)
            {
                return PyObjectToValueVariant(py_result);
            }
            else
            {
                CALL_PY(PyErr_Print)();
                CALL_PY(PyErr_Clear)();
            }
        }
    }

    return PythonValueVariant();
}


bool PythonSupport::setAttribute(PyObjectPtr *object, const std::string &attribute, const PythonValueVariant &value)
{
    int result = 0;

    Python_ThreadBlock thread_block;

    PyObject *py_object = object->get();

    if (py_object)
    {
        PyObject *py_attribute = CALL_PY(PyUnicode_FromString)(attribute.c_str());
        if (py_attribute)
        {
            PyObject *py_value = PythonValueVariantToPyObject(value);
            if (py_value)
            {
                CALL_PY(PyErr_Clear)();
                result = CALL_PY(PyObject_SetAttr)(py_object, py_attribute, py_value);
                PyDecRef(py_value);
            }
            PyDecRef(py_attribute);
        }
    }

    return result != -1;
}

void PythonSupport::scaledImageFromRGBA(PyObject *ndarray_py, unsigned int dest_width, unsigned int dest_height, ImageInterface *image)
{
    Py_buffer array;
    if (CALL_PY(PyObject_GetBuffer)(ndarray_py, &array, PyBUF_ANY_CONTIGUOUS) >= 0)
    {
        long width = array.shape[1];
        long height = array.shape[0];
        if (dest_width < width * 0.75 || dest_height < height * 0.75)
        {
            image->create((int)dest_width, (int)dest_height, ImageFormat::Format_ARGB32);

            int *a_buffer = new int[dest_width];
            int *r_buffer = new int[dest_width];
            int *g_buffer = new int[dest_width];
            int *b_buffer = new int[dest_width];
            long *x_index_buffer = new long[width];
            long *y_index_buffer = new long[height];

            for (int row=0; row<height; ++row)
                y_index_buffer[row] = floor(row / (float(height) / dest_height));

            for (int col=0; col<width; ++col)
                x_index_buffer[col] = floor(col / (float(width) / dest_width));

            long *y_index_ptr = y_index_buffer;

            long last_dst_row = -1;
            long last_row_change = -1;
            for (int row=0; row<height; ++row)
            {
                long dst_row = *y_index_ptr++;
                if (dst_row != last_dst_row)
                {
                    if (dst_row > 0)
                    {
                        uint32_t *dst = (uint32_t *)image->scanLine(last_dst_row);
                        int *a_ptr = a_buffer;
                        int *r_ptr = r_buffer;
                        int *g_ptr = g_buffer;
                        int *b_ptr = b_buffer;
                        int mm =  row - last_row_change;
                        for (int dst_col=0; dst_col<dest_width; ++dst_col)
                        {
                            uint8_t a = *a_ptr++ / mm;
                            uint8_t r = *r_ptr++ / mm;
                            uint8_t g = *g_ptr++ / mm;
                            uint8_t b = *b_ptr++ / mm;
                            *dst++ = (a << 24) | (r << 16) | (g << 8) | b;
                        }
                    }

                    last_dst_row = dst_row;
                    last_row_change = row;

                    memset(a_buffer, 0, dest_width * sizeof(int));
                    memset(r_buffer, 0, dest_width * sizeof(int));
                    memset(g_buffer, 0, dest_width * sizeof(int));
                    memset(b_buffer, 0, dest_width * sizeof(int));
                }

                uint32_t *src = ((uint32_t *)array.buf) + row*width;
                int *a_ptr = a_buffer;
                int *r_ptr = r_buffer;
                int *g_ptr = g_buffer;
                int *b_ptr = b_buffer;
                long *x_index_ptr = x_index_buffer;

                long last_dst_col = -1;
                long last_col_change = -1;
                int a_sum = 0;
                int r_sum = 0;
                int g_sum = 0;
                int b_sum = 0;
                for (int col=0; col<width; ++col)
                {
                    long dst_col = *x_index_ptr++;
                    if (dst_col != last_dst_col)
                    {
                        if (dst_col > 0)
                        {
                            int mm = col - last_col_change;
                            *a_ptr++ += a_sum / mm;
                            *r_ptr++ += r_sum / mm;
                            *g_ptr++ += g_sum / mm;
                            *b_ptr++ += b_sum / mm;
                        }
                        last_dst_col = dst_col;
                        last_col_change = col;
                        uint32_t argb = *src++;
                        a_sum = argb >> 24;
                        r_sum = (argb & 0x00FF0000) >> 16;
                        g_sum = (argb & 0x0000FF00) >> 8;
                        b_sum = argb & 0x000000FF;
                    }
                    else
                    {
                        uint32_t argb = *src++;
                        a_sum += argb >> 24;
                        r_sum += (argb & 0x00FF0000) >> 16;
                        g_sum += (argb & 0x0000FF00) >> 8;
                        b_sum += argb & 0x000000FF;
                    }
                }

                int mm = width - last_col_change;
                *a_ptr += a_sum / mm;
                *r_ptr += r_sum / mm;
                *g_ptr += g_sum / mm;
                *b_ptr += b_sum / mm;
            }

            uint32_t *dst = (uint32_t *)image->scanLine(last_dst_row);
            int *a_ptr = a_buffer;
            int *r_ptr = r_buffer;
            int *g_ptr = g_buffer;
            int *b_ptr = b_buffer;
            int mm =  height - last_row_change;
            for (int dst_col=0; dst_col<dest_width; ++dst_col)
            {
                uint8_t a = *a_ptr++ / mm;
                uint8_t r = *r_ptr++ / mm;
                uint8_t g = *g_ptr++ / mm;
                uint8_t b = *b_ptr++ / mm;
                *dst++ = (a << 24) | (r << 16) | (g << 8) | b;
            }

            delete [] a_buffer;
            delete [] r_buffer;
            delete [] g_buffer;
            delete [] b_buffer;
            delete [] x_index_buffer;
            delete [] y_index_buffer;

            CALL_PY(PyBuffer_Release)(&array);

            // qDebug() << width << "x" << height << " --> " << dest_width << "x" << dest_height;
        }
        else
        {
            image->create((int)width, (int)height, ImageFormat::Format_ARGB32);
            for (int row=0; row<height; ++row)
                memcpy(image->scanLine(row), ((uint32_t *)array.buf) + row*width, width*sizeof(uint32_t));
            CALL_PY(PyBuffer_Release)(&array);
        }
    }
}

void PythonSupport::imageFromRGBA(PyObject *ndarray_py, ImageInterface *image)
{
    Py_buffer array;
    if (CALL_PY(PyObject_GetBuffer)(ndarray_py, &array, PyBUF_ANY_CONTIGUOUS) >= 0)
    {
        long width = array.shape[1];
        long height = array.shape[0];
        image->create((int)width, (int)height, ImageFormat::Format_ARGB32);
        for (int row=0; row<height; ++row)
            memcpy(image->scanLine(row), ((uint32_t *)array.buf) + row*width, width*sizeof(uint32_t));
        CALL_PY(PyBuffer_Release)(&array);
    }
}

void PythonSupport::scaledImageFromArray(PyObject *ndarray_py, float width_, float height_, float context_scaling, float display_limit_low, float display_limit_high, PyObject *lookup_table_ndarray, ImageInterface *image)
{
    Py_buffer array;
    if (CALL_PY(PyObject_GetBuffer)(ndarray_py, &array, PyBUF_ANY_CONTIGUOUS) >= 0)
    {
        long width = array.shape[1];
        long height = array.shape[0];
        float m = display_limit_high != display_limit_low ? 255.0 / (display_limit_high - display_limit_low) : 1;
        std::vector<unsigned int> colorTable;
        if (lookup_table_ndarray != NULL)
        {
            Py_buffer view;
            if (CALL_PY(PyObject_GetBuffer)(lookup_table_ndarray, &view, PyBUF_ANY_CONTIGUOUS) >= 0)
            {
                uint32_t *lookup_table = ((uint32_t *)view.buf);
                for (int i=0; i<256; ++i)
                    colorTable.push_back(lookup_table[i]);
                CALL_PY(PyBuffer_Release)(&view);
            }
        }
        if (colorTable.size() == 0)
            for (int i=0; i<256; ++i)
                colorTable.push_back(0xFF << 24 | i << 16 | i << 8 | i);

        const long dest_width = width_ * context_scaling;
        const long dest_height = height_ * context_scaling;

        if ((width_ * context_scaling < width * 0.75 || height_ * context_scaling < height * 0.75) && (dest_width > 0 && dest_height > 0))
        {
            image->create((int)dest_width, (int)dest_height, ImageFormat::Format_Indexed8);

            float *line_buffer = new float[dest_width];
            long *x_index_buffer = new long[width];
            long *y_index_buffer = new long[height];

            for (int row=0; row<height; ++row)
                y_index_buffer[row] = floor(row / (float(height) / dest_height));

            for (int col=0; col<width; ++col)
                x_index_buffer[col] = floor(col / (float(width) / dest_width));

            long *y_index_ptr = y_index_buffer;

            long last_dst_row = -1;
            long last_row_change = -1;
            for (int row=0; row<height; ++row)
            {
                long dst_row = *y_index_ptr++;
                if (dst_row != last_dst_row)
                {
                    if (dst_row > 0)
                    {
                        uint8_t *dst = (uint8_t *)image->scanLine(last_dst_row);
                        float *line_ptr = line_buffer;
                        float mm =  1.0 / (row - last_row_change);
                        for (int dst_col=0; dst_col<dest_width; ++dst_col)
                        {
                            float v = *line_ptr++ * mm;
                            if (v < display_limit_low)
                                *dst++ = 0x00;
                            else if (v > display_limit_high)
                                *dst++ = 0xFF;
                            else
                                *dst++ = (unsigned char)((v - display_limit_low) * m);
                        }
                    }

                    last_dst_row = dst_row;
                    last_row_change = row;

                    memset(line_buffer, 0, dest_width * sizeof(float));
                }

                float *src = ((float *)array.buf) + row*width;
                float *line_ptr = line_buffer;
                long *x_index_ptr = x_index_buffer;

                long last_dst_col = -1;
                long last_col_change = -1;
                float sum = 0.0;
                for (int col=0; col<width; ++col)
                {
                    long dst_col = *x_index_ptr++;
                    if (dst_col != last_dst_col)
                    {
                        if (dst_col > 0)
                            *line_ptr++ += sum / (col - last_col_change);
                        last_dst_col = dst_col;
                        last_col_change = col;
                        sum = *src++;
                    }
                    else
                    {
                        sum += *src++;
                    }
                }

                *line_ptr += sum / (width - last_col_change);
            }

            uint8_t *dst = (uint8_t *)image->scanLine(last_dst_row);
            float *line_ptr = line_buffer;
            float mm =  1.0 / (height - last_row_change);
            for (int dst_col=0; dst_col<dest_width; ++dst_col)
            {
                float v = *line_ptr++ * mm;
                if (v < display_limit_low)
                    *dst++ = 0x00;
                else if (v > display_limit_high)
                    *dst++ = 0xFF;
                else
                    *dst++ = (unsigned char)((v - display_limit_low) * m);
            }

            delete [] line_buffer;
            delete [] x_index_buffer;
            delete [] y_index_buffer;

            image->setColorTable(colorTable);
            CALL_PY(PyBuffer_Release)(&array);

            // qDebug() << width << "x" << height << " --> " << dest_width << "x" << dest_height;
        }
        else
        {
            image->create((int)width, (int)height, ImageFormat::Format_Indexed8);
            for (int row=0; row<height; ++row)
            {
                float *src = ((float *)array.buf) + row*width;
                uint8_t *dst = (uint8_t *)image->scanLine(row);
                for (int col=0; col<width; ++col)
                {
                    float v = *src++;
                    if (v < display_limit_low)
                        *dst++ = 0x00;
                    else if (v > display_limit_high)
                        *dst++ = 0xFF;
                    else
                        *dst++ = (unsigned char)((v - display_limit_low) * m);
                }
            }
            image->setColorTable(colorTable);
            CALL_PY(PyBuffer_Release)(&array);
        }
    }
}

void PythonSupport::imageFromArray(PyObject *ndarray_py, float display_limit_low, float display_limit_high, PyObject *lookup_table_ndarray, ImageInterface *image)
{
    Py_buffer array;
    if (CALL_PY(PyObject_GetBuffer)(ndarray_py, &array, PyBUF_ANY_CONTIGUOUS) >= 0)
    {
        long width = array.shape[1];
        long height = array.shape[0];
        float m = display_limit_high != display_limit_low ? 255.0 / (display_limit_high - display_limit_low) : 1;
        std::vector<unsigned int> colorTable;
        if (lookup_table_ndarray != NULL)
        {
            Py_buffer view;
            if (CALL_PY(PyObject_GetBuffer)(lookup_table_ndarray, &view, PyBUF_ANY_CONTIGUOUS) >= 0)
            {
                uint32_t *lookup_table = ((uint32_t *)view.buf);
                for (int i=0; i<256; ++i)
                    colorTable.push_back(lookup_table[i]);
                CALL_PY(PyBuffer_Release)(&view);
            }
        }
        if (colorTable.size() == 0)
            for (int i=0; i<256; ++i)
                colorTable.push_back(0xFF << 24 | i << 16 | i << 8 | i);
        if (false)
        {
            const unsigned int *colorTableP = static_cast<const unsigned int *>(colorTable.data());
            image->create((int)width, (int)height, ImageFormat::Format_ARGB32_Premultiplied);
            for (int row=0; row<height; ++row)
            {
                float *src = ((float *)array.buf) + row*width;
                uint32_t *dst = (uint32_t *)image->scanLine(row);
                for (int col=0; col<width; ++col)
                {
                    float v = *src++;
                    if (v < display_limit_low)
                        *dst++ = colorTableP[0];
                    else if (v > display_limit_high)
                        *dst++ = colorTableP[255];
                    else
                    {
                        unsigned char vv = (unsigned char)((v - display_limit_low) * m);
                        *dst++ = colorTableP[vv];
                    }
                }
            }
            CALL_PY(PyBuffer_Release)(&array);
        }
        else
        {
            image->create((int)width, (int)height, ImageFormat::Format_Indexed8);
            for (int row=0; row<height; ++row)
            {
                float *src = ((float *)array.buf) + row*width;
                uint8_t *dst = (uint8_t *)image->scanLine(row);
                for (int col=0; col<width; ++col)
                {
                    float v = *src++;
                    if (v < display_limit_low)
                        *dst++ = 0x00;
                    else if (v > display_limit_high)
                        *dst++ = 0xFF;
                    else
                        *dst++ = (unsigned char)((v - display_limit_low) * m);
                }
            }
            image->setColorTable(colorTable);
            CALL_PY(PyBuffer_Release)(&array);
        }
    }
}

void PythonSupport::arrayFromImage(const ImageInterface &image, PyObject *target)
{
    Py_ssize_t dims[2];
    dims[0] = image.height();
    dims[1] = image.width();

    Py_buffer array;
    if (CALL_PY(PyObject_GetBuffer)(target, &array, PyBUF_ANY_CONTIGUOUS) >= 0)
    {
        long height = array.shape[0];
        long width = array.shape[1];
        for (int row=0; row<height; ++row)
            memcpy(((uint32_t *)array.buf) + row*width, image.scanLine(row), width*sizeof(uint32_t));
        CALL_PY(PyBuffer_Release)(&array);
    }
}

void PythonSupport::shapeFromImage(PyObject *arrayImage, int &width, int &height)
{
    Py_buffer array;
    if (CALL_PY(PyObject_GetBuffer)(arrayImage, &array, PyBUF_ANY_CONTIGUOUS) >= 0)
    {
        height = array.shape[0];
        width = array.shape[1];

        CALL_PY(PyBuffer_Release)(&array);
    }
    else
    {
        height = 0;
        width = 0;
    }
}

void PythonSupport::bufferRelease(Py_buffer *buffer)
{
    CALL_PY(PyBuffer_Release)(buffer);
}

void PythonSupport::setErrorString(const std::string &error_string)
{
    CALL_PY(PyErr_SetString)(module_exception, error_string.c_str());
}

PyObject *PythonSupport::getPyListFromStrings(const std::vector<std::string> &strings)
{
    PyObject *py_list = CALL_PY(PyList_New)(0);

    for (auto str : strings)
    {
        PyObject *py_str = PyString_FromString(str.c_str());
        CALL_PY(PyList_Append)(py_list, py_str); //reference to str stolen
    }

    return py_list;
}

PythonSupport::PyArg_ParseTupleFn PythonSupport::parse()
{
    return dynamic_PyArg_ParseTuple;
}

PythonSupport::Py_BuildValueFn PythonSupport::build()
{
    return dynamic_Py_BuildValue;
}

PyObject *PythonSupport::getNoneReturnValue()
{
    PyIncRef(CALL_PY(Py_NoneGet)());
    return CALL_PY(Py_NoneGet)();
}

bool PythonSupport::isNone(PyObject *obj)
{
    return obj == CALL_PY(Py_NoneGet)();
}

PyObject *PythonSupport::createAndAddModule(PyModuleDef *moduledef)
{
    PyObject *m = CALL_PY(PyModule_Create2)(moduledef, PYTHON_ABI_VERSION); //borrowed reference
    CALL_PY(PyState_AddModule)(m, moduledef);
    return m;
}

void PythonSupport::prepareModuleException(const char *name)
{
    module_exception = CALL_PY(PyErr_NewException)(name, 0, 0);
    PyIncRef(module_exception);
}

void PythonSupport::initializeModule(const char *name, CreateAndAddModuleFn fn)
{
    CALL_PY(PyImport_AppendInittab)(name, fn);
}

void PythonSupport::printAndClearErrors()
{
    if (CALL_PY(PyErr_Occurred)()) { CALL_PY(PyErr_Print)(); CALL_PY(PyErr_Clear)(); }
}

PyObject *PythonSupport::import(const char *name)
{
    return CALL_PY(PyImport_ImportModule)(name);
}

void PyDecRef(PyObject *o) { CALL_PY(Py_DecRef)(o); }
void PyXDecRef(PyObject *o) { if (o != nullptr) CALL_PY(Py_DecRef)(o); }
void PyIncRef(PyObject *o) { CALL_PY(Py_IncRef)(o); }

PythonWChar::PythonWChar(PyObject *o) : _s(nullptr)
{
    _s = CALL_PY(PyUnicode_AsWideCharString)(o, &_size);

}

PythonWChar::~PythonWChar()
{
    CALL_PY(PyMem_Free(_s));
}
