\CONFIG += qt
HEADERS       = mainwindow.h \
                scribblearea.h \
    interactivesegmentation.h \
    StructDefs.h \
    block.h \
    graph.h \
    grabcut.h \
    GMM.h \
    SLIC.h \
    SegmentationThread.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                scribblearea.cpp \
    interactivesegmentation.cpp \
    maxflow.cpp \
    graph.cpp \
    grabcut.cpp \
    GMM.cpp \
    SLIC.cpp \
    SegmentationThread.cpp

# install
target.path = /home/scribble2
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    $$FORMS \
    scribble.pro
sources.path = /home/scribble2
INSTALLS += target \
    sources
symbian:include($$PWD/../../symbianpkgrules.pri)
OTHER_FILES += instances.inc \
    instances.inc


