QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    pipelineconfig.cpp \
    memoryallocator.cpp \
    shaderanalytics.cpp \
    vertexinput.cpp \
    vulkanmain.cpp \
    vulkanrenderer.cpp

HEADERS += \
    filemanager.h \
    mainwindow.h \
    memoryallocator.h \
    pipelineconfig.h \
    reloadflags.h \
    shaderanalytics.h \
    spirvresource.h \
    tiny_obj_loader.h \
    vertexinput.h \
    vulkanmain.h \
    vulkanrenderer.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Lib/release/ -lspirv-cross-core
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Lib/debug/ -lspirv-cross-core

INCLUDEPATH += $$PWD/Lib/Debug

DEPENDPATH += $$PWD/Lib/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/libspirv-cross-core.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/libspirv-cross-core.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/spirv-cross-core.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/spirv-cross-core.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Lib/release/ -lspirv-cross-c
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Lib/debug/ -lspirv-cross-c

INCLUDEPATH += $$PWD/Lib/Debug
DEPENDPATH += $$PWD/Lib/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/libspirv-cross-c.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/libspirv-cross-c.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/spirv-cross-c.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/spirv-cross-c.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Lib/release/ -lspirv-cross-cpp
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Lib/debug/ -lspirv-cross-cpp

INCLUDEPATH += $$PWD/Lib/Debug
DEPENDPATH += $$PWD/Lib/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/libspirv-cross-cpp.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/libspirv-cross-cpp.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/spirv-cross-cpp.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/spirv-cross-cpp.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Lib/release/ -lspirv-cross-glsl
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Lib/debug/ -lspirv-cross-glsl

INCLUDEPATH += $$PWD/Lib/Debug
DEPENDPATH += $$PWD/Lib/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/libspirv-cross-glsl.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/libspirv-cross-glsl.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/spirv-cross-glsl.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/spirv-cross-glsl.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Lib/release/ -lspirv-cross-reflect
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Lib/debug/ -lspirv-cross-reflect

INCLUDEPATH += $$PWD/Lib/Debug
DEPENDPATH += $$PWD/Lib/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/libspirv-cross-reflect.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/libspirv-cross-reflect.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/spirv-cross-reflect.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/spirv-cross-reflect.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/Lib/release/ -lspirv-cross-util
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/Lib/debug/ -lspirv-cross-util

INCLUDEPATH += $$PWD/Lib/Debug
DEPENDPATH += $$PWD/Lib/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/libspirv-cross-util.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/libspirv-cross-util.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/Lib/release/spirv-cross-util.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/Lib/debug/spirv-cross-util.lib

DISTFILES +=
