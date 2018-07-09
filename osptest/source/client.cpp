#include"osp.h"
#include"client.h"

CCApp g_cCApp;

int main(){

#ifdef _MSC_VER
        int ret = OspInit(TRUE,2501,"WindowsOspClient");
#else
        int ret = OspInit(TRUE,2501,"LinuxOspClient");
#endif

        if(!ret){
                OspPrintf(1,0,"osp init fail\n");
        }

        printf("demo client osp\n");
        g_cCApp.CreateApp("OspClientApp",CLIENT_APP_ID,CLIENT_APP_PRI,MAX_MSG_WAITING);
        ret = OspCreateTcpNode(0,OSP_AGENT_CLIENT_PORT);
        if(INVALID_SOCKET == ret){
                OspPrintf(1,0,"create positive node failed,quit\n");
        printf("node created failed\n");
                OspQuit();
                return -1;
        }
        printf("node created successfully\n");
        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::PENDING),CLIENT_ENTRY);
        while(1)
                OspDelay(100);

        OspQuit();
        return 0;
}

void CCInstance::InstanceEntry(CMessage * const pMsg){

        printf("client instance entry\n");
        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
        }

        u32 curState = CurState();
        u16 curEvent = pMsg->event;

        if(curEvent == SIGN_IN_ACK)
                printf("%d\n",curState);
        switch(curState){
                case IDLE_STATE:{
                     switch(curEvent){
                             case CLIENT_ENTRY:{
                                  m_dwdstNode = OspConnectTcpNode(inet_addr("172.16.236.241"),20000,10,3);
                                          if(INVALID_NODE == m_dwdstNode){
                        //                SetTimer(DISCONNET_TIMER, osp_test_interval);
                        //                OspLog(LOG_LVL_KEY, "Connect extern node failed. exit.\n");
                        //                OspTaskResume(g_hMainTask);
                                          return;
                                   }
                                  TSinInfo tSinInfo;
                                  strcpy(tSinInfo.g_Username,"admin");
                                  strcpy(tSinInfo.g_Passwd,"admin");
                                  post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),SERVER_CONNECT_TEST,0,0,m_dwdstNode);
                                  //TODO:登陆命令由GUI发送，后面需要接受
                                  post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_IN,(const void *)&tSinInfo,(u16)sizeof(tSinInfo),m_dwdstNode);
                             }
                             break;
                             //登陆状态回应
                             case SIGN_IN_ACK:{
                                  if(pMsg->length <= 0 || !pMsg->content){
                                          OspLog(SYS_LOG_LEVEL,"sign in failed\n");
                                          OspPrintf(1,1,"sign in failed\n");
                                          printf("sign in failed\n");
                                          //TODO:向GUI发送重新登陆提示
                                  }
                                  if(strcmp((const char*)pMsg->content,"failed") == 0){
                                          OspLog(SYS_LOG_LEVEL,"sign in failed\n");
                                          OspPrintf(1,1,"sign in failed\n");
                                          printf("sign in failed\n");
                                          printf("content:%s\n",pMsg->content);
                                          //TODO:向GUI发送重新登陆提示
                                  }
                                  NextState(RUNNING_STATE);
                                  wDisInsID = pMsg->srcid;
                                  //TODO,应该需要增加其他信息，目前是无法定位的，也有可能log函数自带定位
                                  //，需要确认
                                  OspLog(SYS_LOG_LEVEL,"sign in successfully\n");
                                  OspPrintf(1,1,"sign in successfully\n");
                                  printf("sign in successfully\n");

                                  //测试登出
                                  //TODO
                                  //post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_OUT,NULL,0,m_dwdstNode);
                                  post(wDisInsID,SIGN_OUT,NULL,0,m_dwdstNode);
					
                                  printf("send sign out\n");
                             }
                             break;
                     }
                }
                        break;
                case RUNNING_STATE:
                     switch(curEvent){
                             //TODO：登出命令由GUI发送，根据appid,
                             case SIGN_OUT_CMD:{
                                  post(wDisInsID,SIGN_OUT,NULL,0,m_dwdstNode);
                                  OspLog(SYS_LOG_LEVEL,"get sign out cmd,send to server\n");
                                  OspPrintf(1,1,"get sign out cmd,send to server\n");
                             }
                             break;
                             case SIGN_OUT_ACK:{
                                  NextState(IDLE_STATE);
                                  OspLog(SYS_LOG_LEVEL,"sign out\n");
                                  OspPrintf(1,1,"sign out\n");
printf("get sign out ack\n");
                             }
                             break;
                     }
                        break;
                default:
                        break;
        }

        OspPrintf(1,0,"instance entry\n");
}
