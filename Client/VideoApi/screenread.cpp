#include "screenread.h"
#include"common.h"
ScreenRead::ScreenRead(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT( slot_getScreenFrame()) );
}

ScreenRead::~ScreenRead()
{
    if(m_timer)
    {
        m_timer->stop();
        delete m_timer;
        m_timer = NULL;
    }
}

void ScreenRead::slot_getScreenFrame()
{
    //获取桌面对象
    QScreen *src = QApplication::primaryScreen();
    //获取当前分辨率  得到桌面矩形
    // QRect deskRect = QApplication::desktop()-->screenGeometry();
    //获取桌面图片
    QPixmap map = src->grabWindow( QApplication:: desktop()->winId()/* ,0,0 , deskRect.width() ,deskRect.height()*/);
    QImage image = map.toImage();
    //缩放，改变尺寸
    // image = image.scaled( 1600, 900, Qt::KeepAspectRatio, Qt::SmoothTransformation
    Q_EMIT SIG_getScreenFrame(image);
}

void ScreenRead::slot_openVideo()
{
    m_timer->start(1000/FRAME_RATE - 10);
}

void ScreenRead::slot_closeVideo()
{
    m_timer->stop();
}
