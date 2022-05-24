#ifndef _TCPKERNEL_H
#define _TCPKERNEL_H


#include"block_epoll_net.h"
#include "Mysql.h"


//类成员函数指针 , 用于定义协议映射表
class TcpKernel;
typedef void (TcpKernel::*PFUN)(int,char*,int);


class TcpKernel
{
public:
    //单例模式
    static TcpKernel* GetInstance();

    //开启核心服务
    int Open(int port);
    //初始化随机数
    void initRand();
    //设置协议映射
    void setNetPackMap();
    //关闭核心服务
    void Close();
    //处理网络接收
    static void DealData(int clientfd, char*szbuf, int nlen);
    //事件循环
    void EventLoop();
    //发送数据
    void SendData( int clientfd, char*szbuf, int nlen );

    /************** 网络处理 *********************/
    //注册
    void RegisterRq(int clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(int clientfd, char*szbuf, int nlen);
    //创建房间
    void CreateRoomRq(int clientfd, char*szbuf, int nlen);
    //加入房间
    void JoinRoomRq(int clientfd, char*szbuf, int nlen);
    //离开房间
    void LeaveRoomRq(int clientfd, char*szbuf, int nlen);
    //处理音频帧
    void AudioFrameRq(int clientfd, char*szbuf, int nlen);
    //处理视频帧
    void VideoFrameRq(int clientfd, char*szbuf, int nlen);

    //音频注册
    void AudioRegister(int clientfd, char*szbuf, int nlen);
    //视频注册
    void VideoRegister(int clientfd, char*szbuf, int nlen);
    /*******************************************/
private:
    TcpKernel();
    ~TcpKernel();
    //数据库
    CMysql * m_sql;
    //网络
    Block_Epoll_Net * m_tcp;
    //协议映射表
    PFUN m_NetPackMap[DEF_PACK_COUNT];

    //STL map 不是线程安全的 , 需要加锁避免添加移除元素出现问题
    MyMap< int , UserInfo*> m_mapIDToUserInfo;
    MyMap< int , list<int>> m_mapIDToRoomid;
};

#endif
