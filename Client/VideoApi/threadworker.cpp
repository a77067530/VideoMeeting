#include "threadworker.h"
#include<QDebug>
ThreadWorker::ThreadWorker(QObject *parent) : QObject(parent)
{
    m_pThread = new QThread;
    this->moveToThread(m_pThread);
    m_pThread->start();
}

ThreadWorker::~ThreadWorker()
{
    if(m_pThread)
    {
        m_pThread->quit();
        m_pThread->wait(10);
        if(m_pThread->isRunning())
        {
            m_pThread->terminate();
            m_pThread->wait(10);
        }
        delete m_pThread;
        m_pThread = NULL;
    }
}

worker::~worker()
{
    qDebug()<<"~worker";
}

void worker::slot_doWork()
{
    qDebug()<<"worker:"<<QThread::currentThreadId();
}
