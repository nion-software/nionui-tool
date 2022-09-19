QT += core gui svg widgets
CONFIG += no_keywords

TARGET = NionUILauncher
TEMPLATE = app
QMAKE_CXXFLAGS += -m64 -std=c++11 -Wno-unused-parameter -Wno-unused-variable -DDYNAMIC_PYTHON
INCLUDEPATH += $$(PYTHON_PATH)/include/python3.8 $$(PYTHON_PATH)/lib/python3.8/site-packages/numpy/core/include
SOURCES +=\
    main.cpp \
    DocumentWindow.cpp \
    Application.cpp \
    PythonSelectDialog.cpp \
    PythonStubs.cpp \
    PythonSupport.cpp

HEADERS  += \
    DocumentWindow.h \
    Application.h \
    Image.h \
    PythonSelectDialog.h \
    PythonStubs.h \
    PythonSupport.h

RESOURCES += \
    resources.qrc

# Copy the dynamic library.
unix {
   QMAKE_PRE_LINK = \
      rm -rf linux/x64 ; \
      mkdir -p linux/x64

   QMAKE_LFLAGS_RPATH=
   QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"
}
# message($$OUT_PWD);
unix: LIBS += -L$$OUT_PWD -ldl

INCLUDEPATH += $DESTDIR/include
DEPENDPATH += $DESTDIR/include
