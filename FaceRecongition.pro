#-------------------------------------------------
#
# Project created by QtCreator 2014-07-25T14:49:29
#
#-------------------------------------------------

QT       += core gui

TARGET = FaceRecongition
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        svm_lib/svm.cpp \
        svm_lib/svmUser.cpp \
        detectObject.cpp \
        preprocessFace.cpp

HEADERS  += mainwindow.h \
           svm_lib/svm.h \
           svm_lib/svmUser.h \
           detectObject.h \
           preprocessFace.h

FORMS    += mainwindow.ui

#ifdef __linux__

#elif _WIN32
    INCLUDEPATH += C:\opencv2.3.1_mingw\include
    LIBS += -LC:\\opencv2.3.1_mingw\bin\
        -lopencv_core231 \
        -lopencv_highgui231 \
        -lopencv_imgproc231 \
        -lopencv_features2d231 \
        -lopencv_calib3d231 \
        -lopencv_objdetect231 \
        -lopencv_video231\
        -lopencv_ml231\
        -lopencv_legacy231\
        -lopencv_gpu231\
        -lopencv_flann231\
#else

#endif


RESOURCES += \
    images.qrc


