TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lpthread -lmysqlclient

INCLUDEPATH += ./include

SOURCES += \
    src/Mysql.cpp \
    src/TCPKernel.cpp \
    src/Thread_pool.cpp \
    src/block_epoll_net.cpp \
    src/err_str.cpp \
    src/main.cpp


HEADERS += \
    include/Mysql.h \
    include/TCPKernel.h \
    include/Thread_pool.h \
    include/block_epoll_net.h \
    include/err_str.h \
    include/packdef.h

