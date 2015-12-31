QT += core gui svg widgets
CONFIG += no_keywords

PYTHON_LIB = $(PYTHON_LIB)

TARGET = NionUILauncher
TEMPLATE = app
QMAKE_CXXFLAGS += -m64 -std=c++11
INCLUDEPATH += /usr/include/$$PYTHON_LIB
SOURCES +=\
    main.cpp \
    DocumentWindow.cpp \
    Application.cpp \
    PythonSupport.cpp \

HEADERS  += \
    DocumentWindow.h \
    Application.h \
    PythonSupport.h

RESOURCES += \
    resources.qrc

# Copy the dynamic library.
unix {
   QMAKE_PRE_LINK = \
      rm -rf linux/x64 ; \
      mkdir -p linux/x64
}
# message($$OUT_PWD);
unix: LIBS += -L$$OUT_PWD

INCLUDEPATH += $DESTDIR/include
DEPENDPATH += $DESTDIR/include

unix: LIBS += -l$$PYTHON_LIB
