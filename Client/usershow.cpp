#include "usershow.h"
#include "ui_usershow.h"
#include<QPainter>
#include<QDebug>
UserShow::UserShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserShow),m_defaultImg(":/bq/000.png")
{
    ui->setupUi(this);
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(slot_checkTimer()));

    m_lastTime = QTime::currentTime();

    m_timer.start(1000);
}

UserShow::~UserShow()
{
    delete ui;
}
//设置
void UserShow::slot_setInfo(int id, QString name)
{
    m_id = id;
    m_userName = name;
    QString text = QString("用户:%1").arg(m_userName);
    ui->lb_name->setText(text);
}

//重绘事件
void UserShow::paintEvent(QPaintEvent *event)
{
    //画黑背景
    QPainter painter(this);
    painter.setBrush(Qt::black);//设置黑色画刷
    painter.drawRect( -1,0,this->width()+1,this->height());

    //画视频帧
    //等比例缩放
    m_img = m_img.scaled(this->width(),this->height()-ui->lb_name->height(),Qt::KeepAspectRatio);
    QPixmap pix = QPixmap::fromImage(m_img);
    int x = this->width() - pix.width();
    x = x/2;
    int y = this->height() - pix.height() - ui->lb_name->height();
    y = ui->lb_name->height() + y/2;

    painter.drawPixmap(QPoint(x,y),pix);
    painter.end();
}

void UserShow::slot_setImage(QImage &img)
{
    m_img = img;
    //每次获得画面更新时间
    m_lastTime = QTime::currentTime();
    this->update();//触发重绘
}

void UserShow::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    Q_EMIT SIG_itemClicked(m_id,m_userName);
}

void UserShow::slot_checkTimer()
{
    //定时检测  接收界面是否超时
    if(m_lastTime.secsTo(QTime::currentTime()) > 5)
    {
        //设为默认画面
        slot_setImage(m_defaultImg);
    }
}
