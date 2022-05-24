#include "ckernel.h"
#include"qDebug"
#include<QMessageBox>
#include"md5.h"
#include<QInputDialog>
#include<QRegExp>
#include<QTime>
#define NetPackMap(a) m_netPackMap[a - DEF_PACK_BASE]
//添加协议映射关系
void Ckernel::setNetPackMap()
{
    memset(m_netPackMap,0,sizeof(m_netPackMap));

    //m_netPackMap[DEF_PACK_LOGIN_RS - DEF_PACK_BASE] = &Ckernel::slot_dealLoginRs;
    NetPackMap(DEF_PACK_LOGIN_RS)        = &Ckernel::slot_dealLoginRs;
    NetPackMap(DEF_PACK_REGISTER_RS)     = &Ckernel::slot_dealRegisterRs;
    NetPackMap(DEF_PACK_CREATEROOM_RS)   = &Ckernel::slot_dealCreateRoomRs;
    NetPackMap(DEF_PACK_JOINROOM_RS)   = &Ckernel::slot_dealJoinRoomRs;
    NetPackMap(DEF_PACK_ROOM_MEMBER)   = &Ckernel::slot_dealRoomMemberRq;
    NetPackMap(DEF_PACK_LEAVEROOM_RQ)   = &Ckernel::slot_dealLeaveRoomRq;
    NetPackMap(DEF_PACK_AUDIO_FRAME)   = &Ckernel::slot_dealAudioFrameRq;
    NetPackMap(DEF_PACK_VIDEO_FRAME)   = &Ckernel::slot_dealVideoFrameRq;
}
//加载配置文件
#include<QSettings>
#include<QCoreApplication>
#include<QFileInfo>
void Ckernel::InitConfig()
{
    strcpy(m_serverip,_DEF_SERVERIP);
    //配置文件
    QString path = QCoreApplication::applicationDirPath() + "/config.ini";
    QFileInfo info(path);

    if(info.exists()){
        //如果存在，加载
        //打开配置文件
        QSettings setting(path,QSettings::IniFormat,NULL);
        //跳转到 组 net
        setting.beginGroup("net");
        //获取ip对应的value值
        QVariant var = setting.value("ip");
        QString strip = var.toString();
        if(!strip.isEmpty())
            strcpy(m_serverip,strip.toStdString().c_str());
        //net
        setting.endGroup();

    }else{
        //如果不存在，创建，并写入默认值
        QSettings setting(path,QSettings::IniFormat,NULL);
        //跳转到 组 net
        setting.beginGroup("net");
        //获取ip对应的value值
        setting.setValue("ip",QString::fromStdString(m_serverip));
        //net
        setting.endGroup();
    }

    qDebug()<<"server ip:"<<m_serverip;



}

Ckernel::Ckernel(QObject *parent) : QObject(parent),m_id(0),m_roomid(0)
{

    qDebug()<<"main Thread:"<<QThread::currentThreadId();
    setNetPackMap();
    InitConfig();
    m_pWeChatDlg = new WeChatDialog;
    connect(m_pWeChatDlg,SIGNAL(SIG_close()),this,SLOT(slot_destroy()));
    //    m_pWeChatDlg->show();

    connect(m_pWeChatDlg,SIGNAL(SIG_createRoom()),this,SLOT(slot_createRoom()));
    connect(m_pWeChatDlg,SIGNAL(SIG_joinRoom()),this,SLOT(slot_joinRoom()));

    m_pLoginDlg = new LoginDialog;
    connect(m_pLoginDlg,SIGNAL(SIG_loginCommit(QString,QString)),this,SLOT(slot_loginCommit(QString,QString)));
    m_pLoginDlg->show();
    connect(m_pLoginDlg,SIGNAL(SIG_close()),this,SLOT(slot_destroy()));
    connect(m_pLoginDlg,SIGNAL(SIG_registerCommit(QString,QString,QString)),this,SLOT(slot_registerCommit(QString,QString,QString)));

    m_pRoomdialog = new RoomDialog;
    connect(m_pRoomdialog,SIGNAL(SIG_close()),this,SLOT(slot_quitRoom()));
    connect(m_pRoomdialog,SIGNAL(SIG_audioStart()),this,SLOT(slot_startAudio()));
    connect(m_pRoomdialog,SIGNAL(SIG_audioPause()),this,SLOT(slot_pauseAudio()));

    connect(m_pRoomdialog,SIGNAL(SIG_videoStart()),this,SLOT(slot_startVideo()));
    connect(m_pRoomdialog,SIGNAL(SIG_videoPause()),this,SLOT(slot_pauseVideo()));

    connect(m_pRoomdialog,SIGNAL(SIG_screenStart()),this,SLOT(slot_startScreen()));
    connect(m_pRoomdialog,SIGNAL(SIG_screenPause()),this,SLOT(slot_pauseScreen()));


    //添加网络
    m_pClient = new TcpClientMediator;

    m_pClient->OpenNet(/*_DEF_SERVERIP*/m_serverip,_DEF_PORT);
    connect(m_pClient,SIGNAL(SIG_ReadyData(uint,char*,int)),this,SLOT(slot_dealData(uint,char*,int)));
    for(int i =0;i<2;i++){
        m_pAVClient[i] = new TcpClientMediator;
        m_pAVClient[i]->OpenNet(_DEF_SERVERIP,_DEF_PORT);
        connect(m_pAVClient[i],SIGNAL(SIG_ReadyData(uint,char*,int)),this,SLOT(slot_dealData(uint,char*,int)));
    }

    m_pAudioRead = new AudioRead;
    connect(m_pAudioRead,SIGNAL(SIG_audioFrame(QByteArray)),this,SLOT(slot_audioFrame(QByteArray)));


    m_pVideoRead = new VideoRead;
    connect(m_pVideoRead,SIGNAL(SIG_sendVideoFrame(QImage)),this,SLOT(slot_sendVideoFrame(QImage)));

    m_pScreenRead = new ScreenRead;
    connect(m_pScreenRead,SIGNAL(SIG_getScreenFrame(QImage)),this,SLOT(slot_sendVideoFrame(QImage)));

    m_pSendVideoWorker = QSharedPointer<SendVideoWorker>(new SendVideoWorker);
    connect(this,SIGNAL(SIG_SendVideo(char*,int)),m_pSendVideoWorker.data(),SLOT(slot_sendVideo(char*,int)));

    //设置萌拍效果
    connect(m_pRoomdialog,SIGNAL(SIG_setMoji(int)),m_pVideoRead,SLOT(slot_setMoji(int)));
}



void Ckernel::slot_destroy()
{
    qDebug()<<__func__;

    if(m_pWeChatDlg)
    {
        m_pWeChatDlg->hide();
        delete m_pWeChatDlg;
        m_pWeChatDlg = NULL;
    }
    if(m_pLoginDlg){
        m_pLoginDlg->hide();
        delete m_pLoginDlg;
        m_pLoginDlg = NULL;
    }
    if(m_pAudioRead)
    {
        m_pAudioRead->pause();
        delete m_pAudioRead;
        m_pAudioRead = NULL;
    }
    if(m_pRoomdialog)
    {
        m_pRoomdialog->hide();
        delete m_pRoomdialog;
        m_pRoomdialog = NULL;
    }
    if(m_pClient){
        m_pClient->CloseNet();
        delete m_pClient;
        m_pClient = NULL;
    }
    exit(0);
}

#define MD5_KEY 1234
static std::string GetMD5(QString value)
{
    QString str = QString("%1_%2").arg(value).arg(MD5_KEY);
    std::string strSrc = str.toStdString();
    MD5 md5(strSrc);
    return md5.toString();
}

//提交登录信息
void Ckernel::slot_loginCommit(QString tel, QString pass)
{
    std::string strTel = tel.toStdString();

    STRU_LOGIN_RQ rq;
    strcpy(rq.m_tel,strTel.c_str());

    std::string strPassMD5 = GetMD5(pass);
    qDebug()<<strPassMD5.c_str();
    strcpy(rq.m_password,strPassMD5.c_str());

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
//发送注册信息
void Ckernel::slot_registerCommit(QString tel, QString pass, QString name)
{
    std::string strTel = tel.toStdString();
    std::string strName = name.toStdString();//utf8
    STRU_REGISTER_RQ rq;
    strcpy(rq.m_tel,strTel.c_str());

    std::string strPassMD5 = GetMD5(pass);
    qDebug()<<strPassMD5.c_str();

    //兼容中文utf8 QString -> std::string ->char*
    strcpy(rq.m_name,strName.c_str());
    strcpy(rq.m_password,strPassMD5.c_str());

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
//网络处理
void Ckernel::slot_dealData(uint sock, char *buf, int nlen)
{
    int type = *(int*)buf;
    if(type >= DEF_PACK_BASE && type < DEF_PACK_BASE+DEF_PACK_COUNT)
    {
        //取得协议头，根据协议映射关系，找到函数指针
        PFUN pf = NetPackMap(type);
        if(pf)
        {
            (this->*pf)(sock,buf,nlen);
        }
    }
    delete[] buf;
}
//登陆回复处理
void Ckernel::slot_dealLoginRs(uint sock, char *buf, int nlen)
{
    qDebug()<<__func__;
    //拆包
    STRU_LOGIN_RS *rs = (STRU_LOGIN_RS*)buf;
    //根据返回结果，得到不同信息
    switch (rs->m_lResult) {
    case user_not_exist:
        QMessageBox::about(m_pLoginDlg,"提示","用户不存在，登陆失败");
        break;
    case password_error:
        QMessageBox::about(m_pLoginDlg,"提示","密码错误，登陆失败");
        break;
    case login_success:
    {
        //QString strName = QString("用户[%1]登陆成功").arg(rs->m_name);
        //id 记录
        m_name = QString::fromStdString(rs->m_name);
        m_pWeChatDlg->setInfo(m_name);
        m_id = rs->m_userid;
        //ui跳转
        m_pLoginDlg->hide();
        m_pWeChatDlg->showNormal();

        //注册视频和音频fd
        STRU_AUDIO_REGISTER rq_audio;
        rq_audio.m_userid = m_id;
        STRU_VIDEO_REGISTER rq_video;
        rq_video.m_userid = m_id;

        m_pAVClient[audio_client]->SendData(0,(char*)&rq_audio,sizeof(rq_audio));
        m_pAVClient[video_client]->SendData(0,(char*)&rq_video,sizeof(rq_video));

    }
        break;
    default:
        break;
    }
}
//注册回复处理
void Ckernel::slot_dealRegisterRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_REGISTER_RS *rs = (STRU_REGISTER_RS*)buf;
    //根据不同的结果弹出不同的提示窗
    switch (rs->m_lResult) {
    case tel_is_exist:
        QMessageBox::about(m_pLoginDlg,"提示","手机号已存在，注册失败");
        break;
    case register_success:
        QMessageBox::about(m_pLoginDlg,"提示","注册成功");
        break;
    case name_is_exist:
        QMessageBox::about(m_pLoginDlg,"提示","昵称已存在，注册失败");
        break;
    default:
        break;
    }
}
//创建房间
void Ckernel::slot_createRoom()
{
    //判断是否在房间内 m_roomid
    if(m_roomid != 0 ){
        QMessageBox::about(m_pWeChatDlg,"提示","在房间内，无法加入，先退出");
        return;
    }
    //发命令 创建房间
    STRU_CREATEROOM_RQ rq;
    rq.m_UserID = m_id;
    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
//加入房间
void Ckernel::slot_joinRoom()
{
    //判断是否在房间内 m_roomid
    if(m_roomid != 0 ){
        QMessageBox::about(m_pWeChatDlg,"提示","在房间内，无法加入，先退出");
        return;
    }
    //弹出窗口 填房间号
    QString strRoom = QInputDialog::getText(m_pLoginDlg,"加入房间","输入房间号");
    //合理化判断
    QRegExp exp("^[0-9]\{1,8\}$");
    if(!exp.exactMatch(strRoom)){
        QMessageBox::about(m_pWeChatDlg,"提示","房间号输入不合法，请输入1-8位房间号");
        return;
    }
    qDebug() << strRoom;
    //发命令 加入房间
    STRU_JOINROOM_RQ rq;
    rq.m_UserID = m_id;
    rq.m_RoomID = strRoom.toInt();
    m_pClient->SendData(0,(char*)&rq,sizeof(rq));

}
//退出房间
void Ckernel::slot_quitRoom()
{

    //发退出包
    STRU_LEAVEROOM_RQ rq;
    rq.m_nUserId = m_id;
    rq.m_RoomId = m_roomid;
    std::string name = m_name.toStdString();
    strcpy(rq.szUserName,name.c_str());

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
    //关闭音频 视频
    m_pAudioRead->pause();
    m_pVideoRead->slot_closeVideo();
    m_pScreenRead->slot_closeVideo();
    m_pRoomdialog->slot_setAudioCheck(false);
    m_pRoomdialog->slot_setVideoCheck(false);
    m_pRoomdialog->slot_setScreenCheck(false);
    //回收所有人的audiowrite
    for(auto ite = m_mapIDToAudioWrite.begin();ite != m_mapIDToAudioWrite.end();)
    {
        AudioWrite * pWrite = ite->second;
        ite= m_mapIDToAudioWrite.erase(ite);
        delete pWrite;
    }
    //回收资源
    m_pRoomdialog->slot_clearUserShow();
    m_roomid = 0;
}

void Ckernel::slot_startAudio()
{
    m_pAudioRead->start();
}

void Ckernel::slot_pauseAudio()
{
    m_pAudioRead->pause();
}

//开启视频
void Ckernel::slot_startVideo()
{
    m_pVideoRead->slot_openVideo();
}
//关闭视频
void Ckernel::slot_pauseVideo()
{
    m_pVideoRead->slot_closeVideo();
}

//开启桌面
void Ckernel::slot_startScreen()
{
    m_pScreenRead->slot_openVideo();
}
//关闭桌面
void Ckernel::slot_pauseScreen()
{
    m_pScreenRead->slot_closeVideo();
}

//刷新图片显示
void Ckernel::slot_refreshVideo(int id,QImage &img)
{
    m_pRoomdialog->slot_refreshUser(id , img);
}

///音频数据帧
/// 成员描述
/// int type;
/// int userId;
/// int roomId;
/// int min
/// int sec
/// int msec
/// QByteArray audioFrame;
//发送音频帧
void Ckernel::slot_audioFrame(QByteArray ba)
{
    int nPackSize = 6*sizeof(int) + ba.size();
    char *buf = new char[nPackSize];
    char *tmp = buf;
    //序列化
    int type = DEF_PACK_AUDIO_FRAME;
    int userId = m_id;
    int roomId = m_roomid;
    QTime tm = QTime::currentTime();
    int min  = tm.minute();
    int sec  = tm.second();
    int msec = tm.msec();
    *(int*)tmp = type;
    tmp+=sizeof(int);
    *(int*)tmp = userId;
    tmp+=sizeof(int);
    *(int*)tmp = roomId;
    tmp+=sizeof(int);
    *(int*)tmp = min;
    tmp+=sizeof(int);
    *(int*)tmp = sec;
    tmp+=sizeof(int);
    *(int*)tmp = msec;
    tmp+=sizeof(int);

    memcpy(tmp,ba.data(),ba.size());

    //    m_pClient->SendData(0,buf,nPackSize);
    m_pAVClient[audio_client]->SendData(0,buf,nPackSize);
    delete[] buf;
}
#include<QBuffer>
//发送视频帧
void Ckernel::slot_sendVideoFrame(QImage img)
{
    //显示图片 todo
    slot_refreshVideo(m_id,img);
    //压缩
    //压缩图片从 RGB 24 格式压缩到 JPEG 格式 发送出去
    QByteArray ba;
    QBuffer qbuf(&ba); // QBuffer 与 QByteArray 字节数组联立联系
    img.save( &qbuf , "JPEG" , 50 ); //将图片的数据写入 ba
    //使用 ba 对象 可以获取图片对应的缓冲区
    //可以使用ba.data(),ba.size()将缓冲区发送出去
    //写视频帧 发送

    ///视频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// int min
    /// int sec
    /// int msec
    /// QByteArray videoFrame;
    int nPackSize = 6 * sizeof(int) + ba.size();
    char *buf = new char[nPackSize];
    char *tmp = buf;

    *(int*)tmp = DEF_PACK_VIDEO_FRAME;
    tmp+=sizeof(int);
    *(int*)tmp = m_id;
    tmp+=sizeof(int);
    *(int*)tmp = m_roomid;
    tmp+=sizeof(int);

    //用于延迟过久舍弃一些帧的参考时间
    QTime tm = QTime::currentTime();
    *(int*)tmp = tm.minute();
    tmp+=sizeof(int);
    *(int*)tmp = tm.second();
    tmp+=sizeof(int);
    *(int*)tmp = tm.msec();
    tmp+=sizeof(int);

    memcpy(tmp,ba.data(),ba.size());
    /// 发送是一个阻塞函数，，如果服务器接收缓冲区由于数据量大，没有及时取走缓冲区数据
    /// 滑动窗口变小，send函数阻塞，影响用户界面响应，出现未响应问题
    ///

    //将视频发送 变为一个信号  放到另一个线程  执行
    //    m_pClient->SendData(0,buf,nPackSize);
    //    delete[] buf;
    Q_EMIT SIG_SendVideo(buf,nPackSize);

}
//创建房间回复处理
void Ckernel::slot_dealCreateRoomRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_CREATEROOM_RS *rs = (STRU_CREATEROOM_RS*)buf;

    //房间号 显式到界面 跳转
    m_pRoomdialog->slot_setInfo(QString::number(rs->m_RoomId));
    //服务器没有把个人信息发送给你，作为第一个进入房间的

    //把自己的信息放到房间里 作显式todo
    UserShow *user = new UserShow;
    connect(user,SIGNAL(SIG_itemClicked(int,QString)),m_pRoomdialog,SLOT(slot_setBigImgID(int,QString)));

    user->slot_setInfo(m_id,m_name);
    m_pRoomdialog->slot_addUserShow(user);

    m_roomid = rs->m_RoomId;
    m_pRoomdialog->showNormal();

    //音频初始化
    m_pRoomdialog->slot_setAudioCheck(false);

    //视频初始化
    m_pRoomdialog->slot_setVideoCheck(false);
}
//加入房间回复处理
void Ckernel::slot_dealJoinRoomRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_JOINROOM_RS *rs = (STRU_JOINROOM_RS*)buf;
    //根据结构  失败  提示
    if(rs->m_lResult == 0){
        QMessageBox::about(m_pWeChatDlg,"提示","房间id不存在，加入失败");
        return ;
    }
    //成功
    //房间号 显式到界面 跳转
    m_pRoomdialog->slot_setInfo(QString::number(rs->m_RoomID));
    //跳转 roomid设置
    m_roomid = rs->m_RoomID;
    m_pRoomdialog->showNormal();

    //音频初始化
    m_pRoomdialog->slot_setAudioCheck(false);

    //视频初始化
    m_pRoomdialog->slot_setVideoCheck( false );
}
//房间成员请求处理
void Ckernel::slot_dealRoomMemberRq(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_ROOM_MEMBER_RQ *rq = (STRU_ROOM_MEMBER_RQ*)buf;
    //创建用户对应的控件
    UserShow *user = new UserShow;
    user->slot_setInfo(rq->m_UserID,QString::fromStdString(rq->m_szUser));


    connect(user,SIGNAL(SIG_itemClicked(int,QString)),m_pRoomdialog,SLOT(slot_setBigImgID(int,QString)));
    m_pRoomdialog->slot_addUserShow(user);

    //音频的内容
    AudioWrite *aw = new AudioWrite;
    //为每个人创建一个播放音频的对象
    if(m_mapIDToAudioWrite.count(rq->m_UserID) == 0)
    {
        aw = new AudioWrite;
        m_mapIDToAudioWrite[rq->m_UserID] = aw;
    }

    //视频的内容todo
}
//离开房间的请求处理
void Ckernel::slot_dealLeaveRoomRq(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_LEAVEROOM_RQ *rq = (STRU_LEAVEROOM_RQ*)buf;
    //把这个人从ui上面去掉
    if(rq->m_RoomId == m_roomid)
    {
        m_pRoomdialog->slot_removeUserShow(rq->m_nUserId);
    }

    //去掉对应的音频
    if(m_mapIDToAudioWrite.count(rq->m_nUserId)>0)
    {
        AudioWrite * pAw = m_mapIDToAudioWrite[rq->m_nUserId];
        m_mapIDToAudioWrite.erase(rq->m_nUserId);
        delete pAw;
    }
}

//音频帧处理
void Ckernel::slot_dealAudioFrameRq(uint sock, char *buf, int nlen)
{
    //拆包
    ///音频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// int min
    /// int sec
    /// int msec
    /// QByteArray audioFrame;

    //反序列化
    char *tmp = buf;
    int userId ;
    int roomId ;

    tmp+=sizeof(int);
    userId = *(int*)tmp;
    tmp+=sizeof(int);
    roomId = *(int*)tmp;
    tmp+=sizeof(int);
    //跳过时间
    tmp+=sizeof(int);

    tmp+=sizeof(int);

    tmp+=sizeof(int);

    int nbufLen =nlen -6*sizeof(int);
    QByteArray ba(tmp,nbufLen);

    if(m_roomid == roomId)
    {
        if(m_mapIDToAudioWrite.count(userId) > 0)
        {
            AudioWrite * aw = m_mapIDToAudioWrite[userId];
            aw->slot_playAudio(ba);

        }
    }
}
//视频帧处理
void Ckernel::slot_dealVideoFrameRq(uint sock, char *buf, int nlen)
{
    //拆包
    ///视频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// int min
    /// int sec
    /// int msec
    /// QByteArray videoFrame;
    char *tmp = buf;
    tmp+=sizeof(int);
    int userId = *(int*)tmp;
    tmp+=sizeof(int);
    int roomId = *(int*)tmp;
    tmp+=sizeof(int);

    tmp+=sizeof(int);
    tmp+=sizeof(int);
    tmp+=sizeof(int);

    int datalen = nlen - 6*sizeof(int);
    QByteArray bt(tmp,datalen);
    QImage img;
    img.loadFromData(bt);
    if(m_roomid == roomId)
    {
        m_pRoomdialog->slot_refreshUser(userId,img);
    }
}
//多线程发送视频
void Ckernel::slot_SendVideo(char *buf, int nPackSize)
{
    char *tmp = buf;
    tmp+=sizeof(int);
    tmp+=sizeof(int);
    tmp+=sizeof(int);
    int min = *(int*)tmp;
    tmp+=sizeof(int);
    int sec = *(int*)tmp;
    tmp+=sizeof(int);
    int msec = *(int*)tmp;
    tmp+=sizeof(int);

    //当前时间
    QTime ctm = QTime::currentTime();
    //数据包的时间
    QTime tm(ctm.hour(),min,sec,msec);
    //发送数据包延迟超过300ms舍弃
    if(tm.msecsTo(ctm) > 300){
        qDebug()<<"send fail";
        delete[] buf;
        return;
    }

    //  m_pClient->SendData(0,buf,nPackSize);
    m_pAVClient[video_client]->SendData(0,buf,nPackSize);
    delete[] buf;
}

