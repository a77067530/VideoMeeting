#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include"wechatdialog.h"
#include"TcpClientMediator.h"
#include"packdef.h"
#include"logindialog.h"
#include"roomdialog.h"
#include"audioread.h"
#include"audiowrite.h"

#include"videoread.h"
#include"screenread.h"

#include"threadworker.h"
class Ckernel;
typedef void (Ckernel::*PFUN)(uint sock,char* buf,int nlen);

class SendVideoWorker;
class Ckernel : public QObject
{
    Q_OBJECT
public:
    explicit Ckernel(QObject *parent = nullptr);

    //单例
    static Ckernel *GetInstance()
    {
        static Ckernel kernel;
        return &kernel;
    }
signals:
    void SIG_SendVideo(char *buf,int nPackSize);
public slots:
    void setNetPackMap();

    //加载配置文件
    void InitConfig();


    void slot_destroy();
    //发送登录信息
    void slot_loginCommit(QString tel,QString pass);
    //发送注册信息
    void slot_registerCommit(QString tel,QString pass,QString name);
    //网络信息处理
    void slot_dealData(uint sock,char* buf,int nlen);
    //登陆回复
    void slot_dealLoginRs(uint sock,char* buf,int nlen);
    //注册回复
    void slot_dealRegisterRs(uint sock,char* buf,int nlen);
    //创建房间
    void slot_createRoom();
    //加入房间
    void slot_joinRoom();
    //退出房间
    void slot_quitRoom();

    void slot_startAudio();
    void slot_pauseAudio();
    void slot_startVideo();
    void slot_pauseVideo();

    void slot_startScreen();
    void slot_pauseScreen();

    //刷新图片显示
    void slot_refreshVideo(int id, QImage &img);

    //发送音频帧
    void slot_audioFrame(QByteArray ba);
    //发送视频帧
    void slot_sendVideoFrame(QImage img);
    //创建房间回复处理
    void slot_dealCreateRoomRs(uint sock,char* buf,int nlen );
    //加入房间回复处理
    void slot_dealJoinRoomRs(uint sock,char* buf,int nlen );
    //房间成员请求处理
    void slot_dealRoomMemberRq(uint sock,char* buf,int nlen );
    //离开房间的请求处理
    void slot_dealLeaveRoomRq(uint sock,char* buf,int nlen);
    //音频帧处理
    void slot_dealAudioFrameRq(uint sock,char* buf,int nlen);
    //视频帧处理
    void slot_dealVideoFrameRq(uint sock,char* buf,int nlen);
    //多线程发送视频
    void slot_SendVideo(char *buf,int nPackSize);

private:
    WeChatDialog *m_pWeChatDlg;
    INetMediator *m_pClient;
    LoginDialog *m_pLoginDlg;
    RoomDialog *m_pRoomdialog;
    //协议映射表
    PFUN m_netPackMap[DEF_PACK_COUNT];
    char m_serverip[20];
    int m_id;
    int m_roomid;
    QString m_name;

    ///////////////////////////////////////
    /// 音频   1个采集 多个播放  每一个房间成员 1：1 map映射
    ///
    AudioRead *m_pAudioRead;
    std::map<int,AudioWrite*> m_mapIDToAudioWrite;

    ///////////////////////////////////////
    /// 视频采集
    ///
    VideoRead *m_pVideoRead;

    ScreenRead *m_pScreenRead;

    enum client_type{audio_client = 0,video_client};
    INetMediator *m_pAVClient[2];


    QSharedPointer<SendVideoWorker> m_pSendVideoWorker;

};


class SendVideoWorker : public ThreadWorker
{
    Q_OBJECT
public slots:
    void slot_sendVideo(char *buf,int nlen){
        Ckernel::GetInstance()->slot_SendVideo(buf,nlen);
    }
};

#endif // CKERNEL_H
