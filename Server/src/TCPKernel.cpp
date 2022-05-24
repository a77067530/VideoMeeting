#include<TCPKernel.h>
#include "packdef.h"
#include<stdio.h>
#include<sys/time.h>

using namespace std;

#define NetPackMap(a)  TcpKernel::GetInstance()->m_NetPackMap[ a - DEF_PACK_BASE ]
//设置网络协议映射
void TcpKernel::setNetPackMap()
{
    //清空映射
    bzero( m_NetPackMap , sizeof(m_NetPackMap) );
    //协议映射赋值
    NetPackMap(DEF_PACK_REGISTER_RQ)    = &TcpKernel::RegisterRq;
    NetPackMap(DEF_PACK_LOGIN_RQ)       = &TcpKernel::LoginRq;
    NetPackMap(DEF_PACK_CREATEROOM_RQ)  = &TcpKernel::CreateRoomRq;
    NetPackMap(DEF_PACK_JOINROOM_RQ)    = &TcpKernel::JoinRoomRq;
    NetPackMap(DEF_PACK_LEAVEROOM_RQ)   = &TcpKernel::LeaveRoomRq;
    NetPackMap(DEF_PACK_AUDIO_FRAME)    = &TcpKernel::AudioFrameRq;
    NetPackMap(DEF_PACK_VIDEO_FRAME)    = &TcpKernel::VideoFrameRq;
    NetPackMap(DEF_PACK_AUDIO_REGISTER)    = &TcpKernel::AudioRegister;
    NetPackMap(DEF_PACK_VIDEO_REGISTER)    = &TcpKernel::VideoRegister;
}


TcpKernel::TcpKernel()
{

}

TcpKernel::~TcpKernel()
{

}

TcpKernel *TcpKernel::GetInstance()
{
    static TcpKernel kernel;
    return &kernel;
}

int TcpKernel::Open( int port)
{
    initRand();
    setNetPackMap();
    m_sql = new CMysql;
    // 数据库 使用127.0.0.1 地址  用户名root 密码colin123 数据库 wechat 没有的话需要创建 不然报错
    if(  !m_sql->ConnectMysql("localhost","root","colin123","wechat")  )
    {
        printf("Conncet Mysql Failed...\n");
        return FALSE;
    }
    else
    {
        printf("MySql Connect Success...\n");
    }
    //初始网络
    m_tcp = new Block_Epoll_Net;
    bool res = m_tcp->InitNet( port , &TcpKernel::DealData ) ;
    if( !res )
        err_str( "net init fail:" ,-1);

    return TRUE;
}

void TcpKernel::Close()
{
    m_sql->DisConnect();

}

//随机数初始化
void TcpKernel::initRand()
{
    struct timeval time;
    gettimeofday( &time , NULL);
    srand( time.tv_sec + time.tv_usec );
}

void TcpKernel::DealData(int clientfd,char *szbuf,int nlen)
{
    PackType type = *(PackType*)szbuf;
    if( (type >= DEF_PACK_BASE) && ( type < DEF_PACK_BASE + DEF_PACK_COUNT) )
    {
        PFUN pf = NetPackMap( type );
        if( pf )
        {
            (TcpKernel::GetInstance()->*pf)( clientfd , szbuf , nlen);
        }
    }

    return;
}

void TcpKernel::EventLoop()
{
    printf("event loop\n");
    m_tcp->EventLoop();
}

void TcpKernel::SendData(int clientfd, char *szbuf, int nlen)
{
    m_tcp->SendData(clientfd , szbuf ,nlen );
}



//注册
void TcpKernel::RegisterRq(int clientfd,char* szbuf,int nlen)
{
    printf("clientfd:%d RegisterRq\n", clientfd);
    //1.拆包
    STRU_REGISTER_RQ * rq = (STRU_REGISTER_RQ *)szbuf;
    STRU_REGISTER_RS rs;
    //获取tel password  name

    //查表 t_user 根据tel 查tel
    char sqlStr[1024]={0}/*""*/;
    sprintf( sqlStr , "select tel from t_user where tel = '%s';",rq->m_tel );
    list<string> resList;
    if( !m_sql->SelectMysql(sqlStr , 1, resList ))
    {
        printf("SelectMysql error: %s \n", sqlStr);
        return;
    }
    //有 user存在 返回
    if( resList.size() > 0){
        rs.m_lResult = tel_is_exist;
    }else
    {
        //没有 查表 t_user 根据name 查name  name有没有
        char sqlStr[1024]={0}/*""*/;
        resList.clear();
        sprintf( sqlStr , "select name from t_user where name = '%s';",rq->m_name );

        if( !m_sql->SelectMysql(sqlStr , 1, resList ))
        {
            printf("SelectMysql error: %s \n", sqlStr);
            return;
        }
        if( resList.size() > 0 )
        {   //有 name存在 返回
            rs.m_lResult = name_is_exist;
        }else{
            //没有 写表 tel pass  name 头像和签名的默认值  返回注册成功
            rs.m_lResult = register_success;
            sprintf( sqlStr , "insert into t_user( tel, password , name , icon , feeling) values('%s','%s','%s',%d ,'%s');"
                     ,rq->m_tel,rq->m_password , rq->m_name , 1 , "比较懒,什么也没写" );
            if( !m_sql->UpdataMysql(sqlStr ))
            {
                printf("UpdataMysql error: %s \n", sqlStr);
            }
        }
    }
    m_tcp->SendData( clientfd , (char*)&rs , sizeof(rs) );
}

//登录
void TcpKernel::LoginRq(int clientfd ,char* szbuf,int nlen)
{
    printf("clientfd:%d LoginRq\n", clientfd);

    //拆包  // 手机号 密码
    STRU_LOGIN_RQ * rq = (STRU_LOGIN_RQ *)szbuf;
    STRU_LOGIN_RS rs;
    //根据tel 查pass 和id name
    char strsql[1024] ="";
    list<string> lstRes;
    sprintf( strsql, "select password , id ,name from t_user where tel = '%s';" ,rq->m_tel);
    if( !m_sql->SelectMysql( strsql , 3 , lstRes ) )
    {
        printf("select error:%s\n" , strsql);
        return;
    }
    if( lstRes.size() == 0 ){
        //查不到 返回 没此用户
        rs.m_lResult = user_not_exist;
    }else{
        // 查到了 pass是否一致
        if( strcmp( rq->m_password , lstRes.front().c_str()) != 0 )
        {// 不一致 返回密码错误
            rs.m_lResult = password_error;
        }else{
            lstRes.pop_front();
            //一致   sock 要保存起来  id -> sock 映射 (为了通信)
            int id = atoi( lstRes.front().c_str());
            lstRes.pop_front();

            //sock 要保存起来  id -> sock 映射 (为了通信)
            UserInfo * pInfo = new UserInfo;
            pInfo->m_id = id;
            pInfo->m_roomid = 0;
            pInfo->m_sockfd = clientfd;

            strcpy( pInfo->m_userName  ,  lstRes.front().c_str() );
            lstRes.pop_front();

            // 判断 id 是否在线 , 在线 强制下线, 不在线添加
            if( m_mapIDToUserInfo.IsExist(pInfo->m_id) )
            {
                //强制下线

            }
            m_mapIDToUserInfo.insert( pInfo->m_id , pInfo );
            //写返回包 返回  带id 和结果
            rs.m_userid = id;
            rs.m_lResult = login_success;
            strcpy( rs.m_name  , pInfo->m_userName );
        }
    }
    SendData( clientfd , (char*)&rs , sizeof rs );
}
//创建房间
void TcpKernel::CreateRoomRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d CreateRoomRq\n",clientfd);
    //拆包
    STRU_CREATEROOM_RQ* rq = (STRU_CREATEROOM_RQ*)szbuf;
    //房间号1-8随机数
    int roomid = 0;
    do {
        roomid = rand()%99999999+1;
    } while (m_mapIDToRoomid.IsExist(roomid));
    list<int> lst;
    lst.push_back(rq->m_UserID);
    m_mapIDToRoomid.insert(roomid,lst);
    printf("roomid = %d \n",roomid);
    //随机数 得到房间号，看有没有房间号，可能循环随机 map roomid ->list
    //回复
    STRU_CREATEROOM_RS rs;
    rs.m_RoomId = roomid;
    rs.m_lResult = 1;
    SendData(clientfd,(char*)&rs,sizeof(rs));
}
//加入房间
void TcpKernel::JoinRoomRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d JoinRoomRq\n",clientfd);

    //拆包
    STRU_JOINROOM_RQ *rq = (STRU_JOINROOM_RQ*)szbuf;
    STRU_JOINROOM_RS rs;
    //查看房间是否存在
    if(!m_mapIDToRoomid.IsExist(rq->m_RoomID)){
        //不存在 返回失败
        rs.m_lResult = 0;
        SendData(clientfd,(char*)&rs,sizeof(rs));
        return;
    }
    //存在 返回成功
    rs.m_lResult = 1;
    rs.m_RoomID = rq->m_RoomID;
    SendData(clientfd,(char*)&rs,sizeof(rs));

    if(!m_mapIDToUserInfo.IsExist(rq->m_UserID))
        return;
    UserInfo *joiner = m_mapIDToUserInfo.find(rq->m_UserID);

    STRU_ROOM_MEMBER_RQ joinrq;
    joinrq.m_UserID = rq->m_UserID;
    strcpy(joinrq.m_szUser,joiner->m_userName);

    //发给自己，用于更新
    SendData(clientfd,(char*)&joinrq,sizeof(joinrq));
    //根据房间号拿到房间成员列表
    list<int> lstRoomMem = m_mapIDToRoomid.find(rq->m_RoomID);
    //遍历列表   -- 互换信息
    for(auto ite = lstRoomMem.begin();ite != lstRoomMem.end();ite++){
        int Memid = *ite;
        if(!m_mapIDToUserInfo.IsExist(Memid))
            continue;
        UserInfo *memInfo = m_mapIDToUserInfo.find(Memid);

        STRU_ROOM_MEMBER_RQ memrq;
        memrq.m_UserID = memInfo->m_id;
        strcpy(memrq.m_szUser,memInfo->m_userName);

        //把加入人的信息发给每一个房间内成员
        SendData(memInfo->m_sockfd,(char*)&joinrq,sizeof(joinrq));
        //房间内成员每个人信息发给加入人
        SendData(clientfd,(char*)&memrq,sizeof(memrq));
    }
    //加入人 添加到房间列表
    lstRoomMem.push_back(rq->m_UserID);
    m_mapIDToRoomid.insert(rq->m_RoomID,lstRoomMem);
}
//离开房间
void TcpKernel::LeaveRoomRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d LeaveRoomRq\n",clientfd);
    //拆包
    STRU_LEAVEROOM_RQ *rq =(STRU_LEAVEROOM_RQ*)szbuf;
    //看房间是否存在
    if(!m_mapIDToRoomid.IsExist(rq->m_RoomId))return;

    //如果房间存在，可以获得用户列表
    list<int> lst = m_mapIDToRoomid.find(rq->m_RoomId);
    for(auto ite = lst.begin();ite!=lst.end();)
    {
        int userid = *ite;
    //遍历每个用户，
    //是不是自己，如果是自己，从列表去除 -- 从房间移除了
        if(userid == rq->m_nUserId)
        {
            ite = lst.erase(ite);
        }else{
            //用户是否在线，在线转发
            if(m_mapIDToUserInfo.IsExist(userid))
            {
                UserInfo *info = m_mapIDToUserInfo.find(userid);
                SendData(info->m_sockfd,szbuf,nlen);
            }
        }
    }
    //列表是否节点数为0 -> map项去掉
    if(lst.size() == 0)
    {
        m_mapIDToRoomid.erase(rq->m_RoomId);
        return;
    }
    //更新房间成员列表
    m_mapIDToRoomid.insert(rq->m_RoomId,lst);
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

//处理音频帧
void TcpKernel::AudioFrameRq(int clientfd, char *szbuf, int nlen)
{
    //printf("clientfd:%d AudioFrameRq\n",clientfd);

    //拆包
    char *tmp = szbuf;
    //跳过type
    tmp += sizeof(int);
    //获取userid
    int uid = *(int*)tmp;
    //跳过userid
    tmp+= sizeof(int);
    //获取roomid
    int roomid = *(int*)tmp;
    //看房间是否存在
    if(!m_mapIDToRoomid.IsExist(roomid))return ;
    list<int> lst = m_mapIDToRoomid.find(roomid);
    //获取成员列表
    for(auto ite = lst.begin();ite!=lst.end();ite++)
    {
        //看是否在线  转发
        int userid = *ite;
        //屏蔽掉自己 不转发
        if(uid == userid ) continue;
        if(!m_mapIDToUserInfo.IsExist(userid)) continue;
        UserInfo *userinfo = m_mapIDToUserInfo.find(userid);
        //原样转发
//        SendData(userinfo->m_sockfd,szbuf,nlen);
        SendData(userinfo->m_audiofd,szbuf,nlen);
    }
}
//处理视频帧
void TcpKernel::VideoFrameRq(int clientfd, char *szbuf, int nlen)
{
    //printf("clientfd:%d VideoFrameRq\n",clientfd);

    //拆包
    char *tmp = szbuf;
    //跳过type
    tmp += sizeof(int);
    //获取userid
    int uid = *(int*)tmp;
    //跳过userid
    tmp+= sizeof(int);
    //获取roomid
    int roomid = *(int*)tmp;
    //看房间是否存在
    if(!m_mapIDToRoomid.IsExist(roomid))return ;
    list<int> lst = m_mapIDToRoomid.find(roomid);
    //获取成员列表
    for(auto ite = lst.begin();ite!=lst.end();ite++)
    {
        //看是否在线  转发
        int userid = *ite;
        //屏蔽掉自己 不转发
        if(uid == userid ) continue;
        if(!m_mapIDToUserInfo.IsExist(userid)) continue;
        UserInfo *userinfo = m_mapIDToUserInfo.find(userid);
        //原样转发
        //SendData(userinfo->m_sockfd,szbuf,nlen);
        SendData(userinfo->m_videofd,szbuf,nlen);
    }
}
//音频注册
void TcpKernel::AudioRegister(int clientfd, char *szbuf, int nlen)
{
     printf("clientfd:%d AudioRegister\n",clientfd);

     //拆包
     STRU_AUDIO_REGISTER * rq = (STRU_AUDIO_REGISTER*)szbuf;
     int userid = rq->m_userid;
     //根据userid 找到节点 更新 id
     if(m_mapIDToUserInfo.IsExist(userid))
     {
         UserInfo *info = m_mapIDToUserInfo.find(userid);
         info->m_audiofd = clientfd;
     }
}
//视频注册
void TcpKernel::VideoRegister(int clientfd, char *szbuf, int nlen)
{
     printf("clientfd:%d VideoRegister\n",clientfd);
     //拆包
     STRU_VIDEO_REGISTER * rq = (STRU_VIDEO_REGISTER*)szbuf;
     int userid = rq->m_userid;
     //根据userid 找到节点 更新 id
     if(m_mapIDToUserInfo.IsExist(userid))
     {
         UserInfo *info = m_mapIDToUserInfo.find(userid);
         info->m_videofd = clientfd;
     }
}
