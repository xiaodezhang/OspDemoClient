#include"osp.h"
#include"client.h"

CCApp g_cCApp;

int main(){

#ifdef _MSC_VER
        int ret = OspInit(TRUE,0,"WindowsOspClient");
#else
        int ret = OspInit(TRUE,0,"LinuxOspClient");
#endif

        if(!ret){
                OspPrintf(1,0,"osp init fail\n");
        }

        g_cCApp.CreateApp("OspClientApp",CLIENT_APP_ID,CLIENT_APP_PRI,MAX_MSG_WAITING);
        ret = OspCreateTcpNode(0,OSP_AGENT_CLIENT_PORT);
        if(INVALID_SOCKET == ret){
                OspPrintf(1,0,"create positive node failed,quit\n");
                OspQuit();
                return -1;
        }
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
                                  post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),SERVER_CONNECT_TEST,0,0,m_dwdstNode);
                             }
                             break;
                     }
                }
                        break;
                case RUNNING_STATE:
                        break;
                default:
                        break;
        }

        OspPrintf(1,0,"instance entry\n");
}
