/*
 Copyright (c) 2012-2015 Nion Company.
*/

#include <stdint.h>

// for Q_OS_xyz defs
#include <QtCore/QObject>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QStringList>

#include <QtWidgets/QApplication>

#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON
#if !defined(Q_OS_WIN)
#include <dlfcn.h>
#define LOOKUP_SYMBOL dlsym
#else
#define LOOKUP_SYMBOL GetProcAddress
#endif
#endif

#include "PythonSupport.h"
#include "PythonStubs.h"

#include "Image.h"

#define NPY_NO_DEPRECATED_API NPY_1_19_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL d2af2aa3297e

#pragma push_macro("_DEBUG")
#undef _DEBUG

#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON
// numpy uses these functions, so to allow linking to occur
// override them here and redefine them with the dynamic
// python versions.
#pragma push_macro("PyCapsule_CheckExact")
#pragma push_macro("PyCapsule_GetPointer")
#pragma push_macro("PyErr_Format")
#pragma push_macro("PyErr_Print")
#pragma push_macro("PyErr_SetString")
#pragma push_macro("PyExc_AttributeError")
#pragma push_macro("PyExc_ImportError")
#pragma push_macro("PyExc_RuntimeError")
#pragma push_macro("PyImport_ImportModule")
#pragma push_macro("PyObject_HasAttrString")
#pragma push_macro("PyObject_GetAttrString")
#undef PyCapsule_CheckExact
#define PyCapsule_CheckExact DPyCapsule_CheckExact
#define PyCapsule_GetPointer DPyCapsule_GetPointer
#define PyErr_Format DPyErr_Format
#define PyErr_Print DPyErr_Print
#define PyErr_SetString DPyErr_SetString
#define PyExc_AttributeError DPyExc_GetAttributeError()
#define PyExc_ImportError DPyExc_GetImportError()
#define PyExc_RuntimeError DPyExc_GetRuntimeError()
#define PyImport_ImportModule DPyImport_ImportModule
#define PyObject_GetAttrString DPyObject_GetAttrString
#define PyObject_HasAttrString DPyObject_HasAttrString
#endif

#include "numpy/arrayobject.h"
#include "structmember.h"

static void* init_numpy() {
    import_array();
    return NULL;
}

#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON
#pragma pop_macro("PyCapsule_CheckExact")
#pragma pop_macro("PyCapsule_GetPointer")
#pragma pop_macro("PyErr_Format")
#pragma pop_macro("PyErr_Print")
#pragma pop_macro("PyErr_SetString")
#pragma pop_macro("PyExc_AttributeError")
#pragma pop_macro("PyExc_ImportError")
#pragma pop_macro("PyExc_RuntimeError")
#endif

#pragma pop_macro("_DEBUG")

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <WinBase.h>
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace Qt
{
    static auto endl = ::endl;
}
#endif

static PythonSupport *thePythonSupport = NULL;
const char* PythonSupport::qobject_capsule_name = "b93c9a511d32.qobject";

QString PythonSupport::ensurePython(const QString &python_home)
{
#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON
    QTextStream cout(stdout);

    if (!python_home.isEmpty() && QFile(python_home).exists())
    {
        cout << "Using Python environment: " << python_home << Qt::endl;
        return python_home;
    }
#endif
    return QString();
}

void PythonSupport::initInstance(const std::string &python_home, const std::string &python_library)
{
    thePythonSupport = new PythonSupport(python_home, python_library);
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

class FileSystem
{
public:
    std::string absoluteFilePath(const std::string &dir, const std::string &fileName)
    {
        return QDir(QString::fromStdString(dir)).absoluteFilePath(QString::fromStdString(fileName)).toStdString();
    }

    bool exists(const std::string &filePath)
    {
        return QFile(QString::fromStdString(filePath)).exists();
    }

    std::string toNativeSeparators(const std::string &filePath)
    {
        return QDir::toNativeSeparators(QString::fromStdString(filePath)).toStdString();
    }

    bool parseConfigFile(const std::string &filePath, std::string &home, std::string &version)
    {
        QSettings settings(QString::fromStdString(filePath), QSettings::IniFormat);

        // this code makes me hate both Windows and Qt equally. it is necessary to handle backslashes in paths.
        QFile file(QString::fromStdString(filePath));
        if (file.open(QFile::ReadOnly))
        {
            QByteArray bytes = file.readAll();
            QString str = QString::fromUtf8(bytes);
            Q_FOREACH(const QString &line, str.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts))
            {
                QRegularExpression re("^home\\s?=\\s?(.+)$");
                QRegularExpressionMatch match = re.match(line);
                if (match.hasMatch())
                {
                    QString home_bin_path = match.captured(1).trimmed();
                    if (!home_bin_path.isEmpty())
                    {
                        QString version_str = settings.value("version").toString();
                        QRegularExpression re("(\\d+)\\.(\\d+)(\\.\\d+)?");
                        QRegularExpressionMatch match = re.match(version_str);
                        if (match.hasMatch())
                            version_str = QString::number(match.captured(1).toInt()) + "." + QString::number(match.captured(2).toInt());
                        home = QDir::fromNativeSeparators(home_bin_path).toStdString();
                        version = version_str.toStdString();
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void iterateDirectory(const std::string &directoryPath, const std::list<std::string> &nameFilters, std::list<std::string> &filePaths)
    {
        QStringList nameFiltersQ;
        for (auto nameFilter : nameFilters)
            nameFiltersQ.append(QString::fromStdString(nameFilter));
        QDirIterator it(QString::fromStdString(directoryPath), nameFiltersQ, QDir::NoFilter, QDirIterator::Subdirectories);
        while (it.hasNext())
            filePaths.push_back(it.next().toStdString());
    }

    std::string directoryName(const std::string &filePath)
    {
        QFileInfo fileInfo(QString::fromStdString(filePath));
        if (fileInfo.isDir())
            return fileInfo.fileName().toStdString();
        else
            return fileInfo.absoluteDir().dirName().toStdString();
    }

    std::string directory(const std::string &filePath)
    {
        return QFileInfo(QString::fromStdString(filePath)).absoluteDir().canonicalPath().toStdString();
    }

    std::string parentDirectory(const std::string &filePath)
    {
        QFileInfo fileInfo(QString::fromStdString(filePath));
        if (fileInfo.isDir())
        {
            return fileInfo.absoluteDir().canonicalPath().toStdString();
        }
        else
        {
            QDir d = fileInfo.absoluteDir();
            d.cdUp();
            return d.absolutePath().toStdString();
        }
    }

    void putEnv(const std::string &key, const std::string &value)
    {
        qputenv(key.c_str(), value.c_str());
    }

    std::string getEnv(const std::string &key)
    {
        return QString::fromLocal8Bit(qgetenv(key.c_str())).toStdString();
    }
};

#if defined(Q_OS_MAC)
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

        std::list<std::string> variants;
        variants.push_back("libpython3.11.dylib");
        variants.push_back("libpython3.10.dylib");
        variants.push_back("libpython3.9.dylib");
        variants.push_back("libpython3.8.dylib");

        for (auto directory: directories)
        {
            std::list<std::string> directoryFilePaths;
            fs->iterateDirectory(directory, variants, directoryFilePaths);
            for (auto filePath : directoryFilePaths)
                if (fs->directoryName(filePath) == "lib")
                    filePaths.push_back(filePath);
        }
    }

    virtual void buildStandardPaths(FileSystem *fs, const std::string &python_home, std::list<std::string> &filePaths) override
    {
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.11.dylib"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.10.dylib"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.9.dylib"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.8.dylib"));
    }

    virtual std::string findLandmarkLibrary(FileSystem *fs, const std::string &filePath) override
    {
        return fs->parentDirectory(filePath);
    }
};
#endif

#if defined(Q_OS_LINUX)
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
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/python3.11/config-3.11-x86_64-linux-gnu/libpython3.11.so"));
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.so"));
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/python3.9/config-3.9-x86_64-linux-gnu/libpython3.9.so"));
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/python3.8/config-3.8-x86_64-linux-gnu/libpython3.8.so"));
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/libpython3.11.so"));
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/libpython3.10.so"));
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/libpython3.9.so"));
        filePaths.push_back(fs->absoluteFilePath(homeParentDirectory, "lib/libpython3.8.so"));
    }

    virtual void buildStandardPaths(FileSystem *fs, const std::string &python_home, std::list<std::string> &filePaths) override
    {
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.11.so"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.10.so"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.9.so"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "lib/libpython3.8.so"));
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
};
#endif

#if defined(Q_OS_WIN)
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
        filePaths.push_back(fs->absoluteFilePath(python_home, "Scripts/Python311.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python311.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Scripts/Python311.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Python311.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Scripts/Python310.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python310.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Scripts/Python310.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Python310.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Scripts/Python39.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python39.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Scripts/Python39.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Python39.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Scripts/Python38.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python38.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Scripts/Python38.dll"));
        filePaths.push_back(fs->absoluteFilePath(home_bin_path, "Python38.dll"));
    }

    virtual void buildStandardPaths(FileSystem *fs, const std::string &python_home, std::list<std::string> &filePaths) override
    {
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python311.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python310.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python39.dll"));
        filePaths.push_back(fs->absoluteFilePath(python_home, "Python38.dll"));
    }

    virtual std::string findLandmarkLibrary(FileSystem *fs, const std::string &filePath) override
    {
        return fs->directory(filePath);
    }
};
#endif

PlatformSupport::~PlatformSupport()
{
}

PythonSupport::PythonSupport(const std::string &python_home, const std::string &python_library)
    : module_exception(NULL)
{
#if defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON
#if defined(Q_OS_MAC)
    ps = std::unique_ptr<PlatformSupport>(new MacSupport());
#elif defined(Q_OS_LINUX)
    ps = std::unique_ptr<PlatformSupport>(new LinuxSupport());
#else
    ps = std::unique_ptr<PlatformSupport>(new WinSupport());
#endif
    std::unique_ptr<FileSystem> fs(new FileSystem());
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

    m_actual_python_home = QString::fromStdString(ps->findLandmarkLibrary(fs.get(), filePath));

    ps->loadLibrary(fs.get(), python_home, filePath);

    m_valid = ps->isValid();

    dynamic_PyArg_ParseTuple = (PyArg_ParseTupleFn)ps->lookupSymbol("PyArg_ParseTuple");
    dynamic_Py_BuildValue = (Py_BuildValueFn)ps->lookupSymbol("Py_BuildValue");
#else
    dynamic_PyArg_ParseTuple = PyArg_ParseTuple;
    dynamic_Py_BuildValue = Py_BuildValue;
#endif
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
    Py_XDECREF(module_exception);
}

Q_DECLARE_METATYPE(PyObjectPtr)

// static
int PyObjectPtr::metaId()
{
    static bool once = false;
    static int meta_id = 0;
    if (!once)
    {
        meta_id = qRegisterMetaType<PyObjectPtr>("PyObjectPtr");
        once = true;
    }
    return meta_id;
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

static wchar_t python_home_static[512];
static wchar_t python_program_name_static[512];

void PythonSupport::initialize(const QString &python_home, const QList<QString> &python_paths, const QString &python_library)
{
#if !(defined(DYNAMIC_PYTHON) && DYNAMIC_PYTHON)
#if defined(Q_OS_MAC) && !defined(DEBUG)
    if (!python_home.isEmpty())
        setenv("PYTHONHOME", python_home.toUtf8().constData(), 1);
#endif
#else
    if (!python_home.isEmpty())
    {
        memset(&python_home_static[0], 0, sizeof(python_home_static));
        python_home.toWCharArray(python_home_static);
        CALL_PY(Py_SetPythonHome)(python_home_static);  // requires a permanent buffer
    }
#endif

#if defined(Q_OS_MAC)
    QString python_home_new = python_home;
    QString python_program_name = QDir(python_home).absoluteFilePath("bin/python3");

    // check if we're running inside a venv, determined by whether pyvenv.cfg exists.
    // if so, read the config file and find the home key, indicating the python installation
    // directory (with /bin attached). then call SetPythonHome and SetProgramName with the
    // installation directory and virtual environment python path respectively. these are
    // required for the virtual environment to load correctly.
    QString venv_conf_file_name = QDir(python_home).absoluteFilePath("pyvenv.cfg");
    if (QFile(venv_conf_file_name).exists())
    {
        python_home_new = m_actual_python_home;
    }

    if (!python_paths.isEmpty())
        CALL_PY(Py_SetPath)(const_cast<wchar_t *>(python_paths.join(":").toStdWString().c_str()));

    memset(&python_home_static[0], 0, sizeof(python_home_static));
    python_home_new.toWCharArray(python_home_static);
    CALL_PY(Py_SetPythonHome)(python_home_static);  // requires a permanent buffer

    memset(&python_program_name_static[0], 0, sizeof(python_program_name_static));
    python_program_name.toWCharArray(python_program_name_static);
    CALL_PY(Py_SetProgramName)(python_program_name_static);  // requires a permanent buffer
#elif defined(Q_OS_WIN)
    QString python_home_new = python_home;
    QString python_program_name = QDir(python_home).absoluteFilePath("Python.exe");

    // check if we're running inside a venv, determined by whether pyvenv.cfg exists.
    // if so, read the config file and find the home key, indicating the python installation
    // directory (with /bin attached). then call SetPythonHome and SetProgramName with the
    // installation directory and virtual environment python path respectively. these are
    // required for the virtual environment to load correctly.
    QString venv_conf_file_name = QDir(python_home).absoluteFilePath("pyvenv.cfg");
    if (QFile(venv_conf_file_name).exists())
    {
        // probably Python w/ virtual environment.
        // this code makes me hate both Windows and Qt equally. it is necessary to handle backslashes in paths.
        QFile file(venv_conf_file_name);
        if (file.open(QFile::ReadOnly))
        {
            QByteArray bytes = file.readAll();
            QString str = QString::fromUtf8(bytes);
            Q_FOREACH(const QString &line, str.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts))
            {
                QRegularExpression re("^home\\s?=\\s?(.+)$");
                QRegularExpressionMatch match = re.match(line);
                if (match.hasMatch())
                {
                    QString home_bin_path = match.captured(1).trimmed();
                    if (!home_bin_path.isEmpty())
                    {
                        python_home_new = m_actual_python_home;

                        python_program_name = QDir(python_home).absoluteFilePath("Python.exe");

                        // required to configure the path; see https://bugs.python.org/issue34725
                        QStringList python_paths;
                        python_paths.append(QDir(python_home).absoluteFilePath("Scripts/python311.zip"));
                        python_paths.append(QDir(python_home).absoluteFilePath("Scripts/python310.zip"));
                        python_paths.append(QDir(python_home).absoluteFilePath("Scripts/python39.zip"));
                        python_paths.append(QDir(python_home).absoluteFilePath("Scripts/python38.zip"));
                        python_paths.append(QDir(python_home_new).absoluteFilePath("DLLs"));
                        python_paths.append(QDir(python_home_new).absoluteFilePath("lib"));
                        python_paths.append(QDir(python_home_new).absolutePath());
                        python_paths.append(QDir(python_home).absolutePath());
                        python_paths.append(QDir(python_home).absoluteFilePath("lib/site-packages"));
                        CALL_PY(Py_SetPath)(const_cast<wchar_t *>(python_paths.join(";").toStdWString().c_str()));
                    }
                }
            }
        }
    }

    if (!python_paths.isEmpty())
        CALL_PY(Py_SetPath)(const_cast<wchar_t *>(python_paths.join(";").toStdWString().c_str()));

    memset(&python_home_static[0], 0, sizeof(python_home_static));
    python_home_new.toWCharArray(python_home_static);
    CALL_PY(Py_SetPythonHome)(python_home_static);  // requires a permanent buffer

    memset(&python_program_name_static[0], 0, sizeof(python_program_name_static));
    python_program_name.toWCharArray(python_program_name_static);
    CALL_PY(Py_SetProgramName)(python_program_name_static);  // requires a permanent buffer
#elif defined(Q_OS_LINUX)
    QString python_home_new = python_home;
    QString python_program_name = QDir(python_home).absoluteFilePath("bin/python3");

    // check if we're running inside a venv, determined by whether pyvenv.cfg exists.
    // if so, read the config file and find the home key, indicating the python installation
    // directory (with /bin attached). then call SetPythonHome and SetProgramName with the
    // installation directory and virtual environment python path respectively. these are
    // required for the virtual environment to load correctly.
    QString venv_conf_file_name = QDir(python_home).absoluteFilePath("pyvenv.cfg");
    if (QFile(venv_conf_file_name).exists())
    {
        python_home_new = m_actual_python_home;
    }

    if (!python_paths.isEmpty())
        CALL_PY(Py_SetPath)(const_cast<wchar_t *>(python_paths.join(":").toStdWString().c_str()));

    memset(&python_home_static[0], 0, sizeof(python_home_static));
    python_home_new.toWCharArray(python_home_static);
    CALL_PY(Py_SetPythonHome)(python_home_static);  // requires a permanent buffer

    memset(&python_program_name_static[0], 0, sizeof(python_program_name_static));
    python_program_name.toWCharArray(python_program_name_static);
    CALL_PY(Py_SetProgramName)(python_program_name_static);  // requires a permanent buffer
#endif

    CALL_PY(Py_Initialize)();

    // release the GIL. the tool will normally run with the GIL released. calls back to Python
    // will need to acquire the GIL. the initial state is saved because the GIL is required to
    // finalize.
    m_initial_state = CALL_PY(PyEval_SaveThread)();

    PyObjectPtr::metaId();

    Python_ThreadBlock thread_block;

    init_numpy();
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
        Py_INCREF(CALL_PY(Py_NoneGet)());
        return CALL_PY(Py_NoneGet)();
    }
    else if (std::holds_alternative<bool>(value_variant.value))
    {
        PyObject *py_obj = *std::get_if<bool>(&value_variant.value) ? CALL_PY(Py_TrueGet)() : CALL_PY(Py_FalseGet)();
        Py_INCREF(py_obj);
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
            Py_DECREF(py_key);
            Py_DECREF(py_value);
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
        Py_INCREF(py_object);
        return py_object;
    }

    Py_INCREF(CALL_PY(Py_NoneGet)());
    return CALL_PY(Py_NoneGet)();
}

QString PyObjectToQString(PyObject* val)
{
    if (PyUnicode_Check(val))
    {
        return QString::fromUtf8(CALL_PY(PyUnicode_AsUTF8)(val));
    }
    return QString();
}

PythonValueVariant PyObjectToValueVariant(PyObject *py_object)
{
    if (PyString_Check(py_object) || PyUnicode_Check(py_object))
    {
        return PythonValueVariant{std::string(CALL_PY(PyUnicode_AsUTF8)(py_object))};
    }
    else if (PyInt_Check(py_object))
    {
        return PythonValueVariant{PyInt_AsLong(py_object)};
    }
    else if (PyLong_Check(py_object))
    {
        return PythonValueVariant{CALL_PY(PyLong_AsLongLong)(py_object)};
    }
    else if (CALL_PY(PyFloat_Check)(py_object))
    {
        return PythonValueVariant{CALL_PY(PyFloat_AsDouble)(py_object)};
    }
    else if (CALL_PY(PyBool_Check)(py_object))
    {
        return PythonValueVariant{static_cast<bool>(CALL_PY(PyObject_IsTrue)(py_object))};
    }
    else if (CALL_PY(PyCapsule_IsValid)(py_object, PythonSupport::qobject_capsule_name) && CALL_PY(PyCapsule_CheckExact)(py_object))
    {
        return PythonValueVariant{CALL_PY(PyCapsule_GetPointer)(py_object, PythonSupport::qobject_capsule_name)};
    }
    else if (PyDict_Check(py_object))
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
            Py_DECREF(items);
        }
        return PythonValueVariant{map};
    }
    else if ((PyList_Check(py_object) || PyTuple_Check(py_object)) && CALL_PY(PySequence_Check)(py_object))
    {
        std::vector<PythonValueVariant> list;
        int count = (int)CALL_PY(PySequence_Size)(py_object);
        PyObject *fast_list = CALL_PY(PySequence_Fast)(py_object, "error");
        PyObject **fast_items = PySequence_Fast_ITEMS(fast_list);
        for (int i=0; i<count; i++)
        {
            list.push_back(PyObjectToValueVariant(fast_items[i]));
        }
        Py_DECREF(fast_list);
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
    Py_DECREF(py_filename);
    Py_DECREF(py_path);
    Py_DECREF(sys_module);
}

PythonValueVariant PythonSupport::invokePyMethod(PyObjectPtr *object, const QString &method, const std::list<PythonValueVariant> &args)
{
    Python_ThreadBlock thread_block;

    PyObject *py_object = object->get();

    if (py_object)
    {
        PyObjectPtr callable(CALL_PY(PyObject_GetAttrString)(py_object, method.toLatin1().data()));
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
                Py_DECREF(py_value);
            }
            Py_DECREF(py_attribute);
        }
    }

    return result != -1;
}

void PythonSupport::scaledImageFromRGBA(PyObject *ndarray_py, const QSize &destination_size, ImageInterface *image)
{
    PyArrayObject *array = (PyArrayObject *)PyArray_ContiguousFromObject(ndarray_py, NPY_UINT32, 2, 2);
    if (array != NULL)
    {
        long width = PyArray_DIMS(array)[1];
        long height = PyArray_DIMS(array)[0];
        if (destination_size.width() < width * 0.75 || destination_size.height() < height * 0.75)
        {
            const long dest_width = destination_size.width();
            const long dest_height = destination_size.height();

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

                uint32_t *src = ((uint32_t *)PyArray_DATA(array)) + row*width;
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

            Py_DECREF(array);

            // qDebug() << width << "x" << height << " --> " << dest_width << "x" << dest_height;
        }
        else
        {
            image->create((int)width, (int)height, ImageFormat::Format_ARGB32);
            for (int row=0; row<height; ++row)
                memcpy(image->scanLine(row), ((uint32_t *)PyArray_DATA(array)) + row*width, width*sizeof(uint32_t));
            Py_DECREF(array);
        }
    }
}

void PythonSupport::imageFromRGBA(PyObject *ndarray_py, ImageInterface *image)
{
    PyArrayObject *array = (PyArrayObject *)PyArray_ContiguousFromObject(ndarray_py, NPY_UINT32, 2, 2);
    if (array != NULL)
    {
        long width = PyArray_DIMS(array)[1];
        long height = PyArray_DIMS(array)[0];
        image->create((int)width, (int)height, ImageFormat::Format_ARGB32);
        for (int row=0; row<height; ++row)
            memcpy(image->scanLine(row), ((uint32_t *)PyArray_DATA(array)) + row*width, width*sizeof(uint32_t));
        Py_DECREF(array);
    }
}

void PythonSupport::scaledImageFromArray(PyObject *ndarray_py, const QSizeF &destination_size, float context_scaling, float display_limit_low, float display_limit_high, PyObject *lookup_table_ndarray, ImageInterface *image)
{
    PyArrayObject *array = (PyArrayObject *)PyArray_ContiguousFromObject(ndarray_py, NPY_FLOAT32, 2, 2);
    if (array != NULL)
    {
        long width = PyArray_DIMS(array)[1];
        long height = PyArray_DIMS(array)[0];
        float m = display_limit_high != display_limit_low ? 255.0 / (display_limit_high - display_limit_low) : 1;
        std::vector<unsigned int> colorTable;
        if (lookup_table_ndarray != NULL)
        {
            PyArrayObject *lookup_table_array = (PyArrayObject *)PyArray_ContiguousFromObject(lookup_table_ndarray, NPY_UINT32, 1, 1);
            if (lookup_table_array != NULL)
            {
                uint32_t *lookup_table = ((uint32_t *)PyArray_DATA(lookup_table_array));
                for (int i=0; i<256; ++i)
                    colorTable.push_back(lookup_table[i]);
                Py_DECREF(lookup_table_array);
            }
        }
        if (colorTable.size() == 0)
            for (int i=0; i<256; ++i)
                colorTable.push_back(0xFF << 24 | i << 16 | i << 8 | i);

        const long dest_width = destination_size.width() * context_scaling;
        const long dest_height = destination_size.height() * context_scaling;

        if ((destination_size.width() * context_scaling < width * 0.75 || destination_size.height() * context_scaling < height * 0.75) && (dest_width > 0 && dest_height > 0))
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

                float *src = ((float *)PyArray_DATA(array)) + row*width;
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
            Py_DECREF(array);

            // qDebug() << width << "x" << height << " --> " << dest_width << "x" << dest_height;
        }
        else
        {
            image->create((int)width, (int)height, ImageFormat::Format_Indexed8);
            for (int row=0; row<height; ++row)
            {
                float *src = ((float *)PyArray_DATA(array)) + row*width;
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
            Py_DECREF(array);
        }
    }
}

void PythonSupport::imageFromArray(PyObject *ndarray_py, float display_limit_low, float display_limit_high, PyObject *lookup_table_ndarray, ImageInterface *image)
{
    PyArrayObject *array = (PyArrayObject *)PyArray_ContiguousFromObject(ndarray_py, NPY_FLOAT32, 2, 2);
    if (array != NULL)
    {
        long width = PyArray_DIMS(array)[1];
        long height = PyArray_DIMS(array)[0];
        float m = display_limit_high != display_limit_low ? 255.0 / (display_limit_high - display_limit_low) : 1;
        std::vector<unsigned int> colorTable;
        if (lookup_table_ndarray != NULL)
        {
            PyArrayObject *lookup_table_array = (PyArrayObject *)PyArray_ContiguousFromObject(lookup_table_ndarray, NPY_UINT32, 1, 1);
            if (lookup_table_array != NULL)
            {
                uint32_t *lookup_table = ((uint32_t *)PyArray_DATA(lookup_table_array));
                for (int i=0; i<256; ++i)
                    colorTable.push_back(lookup_table[i]);
                Py_DECREF(lookup_table_array);
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
                float *src = ((float *)PyArray_DATA(array)) + row*width;
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
            Py_DECREF(array);
        }
        else
        {
            image->create((int)width, (int)height, ImageFormat::Format_Indexed8);
            for (int row=0; row<height; ++row)
            {
                float *src = ((float *)PyArray_DATA(array)) + row*width;
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
            Py_DECREF(array);
        }
    }
}

PyObject *PythonSupport::arrayFromImage(const ImageInterface &image)
{
    Py_ssize_t dims[2];
    dims[0] = image.height();
    dims[1] = image.width();

    PyObject *obj = PyArray_SimpleNew(2, dims, NPY_UINT32);

    PyArrayObject *array = (PyArrayObject *)PyArray_ContiguousFromObject(obj, NPY_UINT32, 2, 2);

    if (array != NULL)
    {
        long height = PyArray_DIMS(array)[0];
        long width = PyArray_DIMS(array)[1];
        for (int row=0; row<height; ++row)
            memcpy(((uint32_t *)PyArray_DATA(array)) + row*width, image.scanLine(row), width*sizeof(uint32_t));

        Py_DECREF(array);

        return obj;
    }
    else
    {
        Py_DECREF(obj);

        return NULL;
    }
}

void PythonSupport::bufferRelease(Py_buffer *buffer)
{
    CALL_PY(PyBuffer_Release)(buffer);
}

void PythonSupport::setErrorString(const QString &error_string)
{
    CALL_PY(PyErr_SetString)(module_exception, error_string.toUtf8().constData());
}

PyObject *PythonSupport::getPyListFromStrings(const QStringList &strings)
{
    PyObject *py_list = CALL_PY(PyList_New)(0);

    Q_FOREACH(const QString &str, strings)
    {
        PyObject *py_str = PyString_FromString(str.toUtf8().data());
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
    Py_INCREF(CALL_PY(Py_NoneGet)());
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
    Py_INCREF(module_exception);
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

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION == 8
// work around mis-defined macros in cpython/objects.h in Python 3.8 headers.
// the macro isn't defined at first use (in cpython/objects.h) so it is
// declared as a function. since we aren't linking to the python lib,
// the function is missing. define it here.
// see https://bugs.python.org/issue39543
// see https://github.com/python/cpython/pull/18361/files
#undef _Py_Dealloc
PyAPI_FUNC(void) _Py_Dealloc(PyObject *o) { _Py_Dealloc_inline(o); }
#endif

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
// work around to provide required function that would be available by linking.
#if defined(Q_OS_WIN)
#pragma warning(push)
#pragma warning(disable: 4273)  // do not warn about conflicting dllimport vs dllexport dll linkage.
void _Py_Dealloc(PyObject* o) { (*(Py_TYPE(o)->tp_dealloc))(o); }
#pragma warning(pop)
#else
PyAPI_FUNC(void) _Py_Dealloc(PyObject *o) { (*(Py_TYPE(o)->tp_dealloc))(o); }
#endif
#endif
