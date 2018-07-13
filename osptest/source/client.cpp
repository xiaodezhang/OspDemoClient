#include"osp.h"
#include"client.h"

CCApp g_cCApp;
static s8 chCancleFlag = 0;

#define MAKEESTATE(state,event) ((u32)(event<< 4 +state))
#define SERVER_IP                ("172.16.236.241")
#define SERVER_PORT              (20000)

API void SendSignInCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),CLIENT_ENTRY);
}

API void SendSignOutCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_OUT_CMD);
}



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

#ifdef _LINUX_
        OspRegCommand("SignIn",(void*)SendSignInCmd,"");
        OspRegCommand("SignOut",(void*)SendSignOutCmd,"");
#endif
        while(1)
                OspDelay(100);

        OspQuit();
        return 0;
}


void test_file_send(){

        s8 file_name[] = "test_file_name";

        ::OspPost(MAKEIID(CLIENT_APP_ID,CLIENT_INSTANCE_ID),FILE_NAME_SEND_CMD,
                        file_name,strlen(file_name)+1);
}

u32 CCInstance::GetDstNode(){

        return m_dwdstNode;
}
void CCInstance::FileSendCmd2Client(){


#if 0
    const char file_name_path[] = "test_file_name";
    FILE *file;
    size_t buffer_size;

    if(!(file = fopen(file_name_path,"rb"))){
          printf("open file error\n");
          return;
    }
    buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file);
    post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_UPLOAD
                                      ,buffer,buffer_size,GetDstNode());

    while((buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file)) > 0){
          //TODO:Instance实例选择需要修改,增加中断变量
        post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_UPLOAD
                        ,buffer,buffer_size,m_dwdstNode);
    }
#endif
}

#if 1

typedef struct tagCmdNode{
        u32         EventState;
        CCInstance::MsgProcess  c_MsgProcess;
        struct      tagCmdNode *next;
}tCmdNode;


void CCInstance::ClientEntry(CMessage *const pMsg){

          m_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
          if(INVALID_NODE == m_dwdstNode){
//                SetTimer(DISCONNET_TIMER, osp_test_interval);
//                OspLog(LOG_LVL_KEY, "Connect extern node failed. exit.\n");
//                OspTaskResume(g_hMainTask);
                  return;
          }
          TSinInfo tSinInfo;
          strcpy(tSinInfo.g_Username,"admin");
          strcpy(tSinInfo.g_Passwd,"admin");
          post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_IN,(const void *)&tSinInfo,(u16)sizeof(tSinInfo),m_dwdstNode);
}

void CCInstance::SignInAck(CMessage * const pMsg){
          if(pMsg->length <= 0 || !pMsg->content){
                  OspLog(SYS_LOG_LEVEL,"sign in failed\n");
                  OspPrintf(1,1,"sign in failed\n");
                  printf("sign in failed\n");
                  //TODO:向GUI发送重新登陆提示
          }
          if(strcmp((LPCSTR)pMsg->content,"failed") == 0){
                  OspLog(SYS_LOG_LEVEL,"sign in failed\n");
                  OspPrintf(1,1,"sign in failed\n");
                  printf("sign in failed\n");
                  printf("content:%s\n",pMsg->content);
                  //TODO:向GUI发送重新登陆提示
          }
          NextState(RUNNING_STATE);
          //TODO,应该需要增加其他信息，目前是无法定位的，也有可能log函数自带定位
          //，需要确认
          OspLog(SYS_LOG_LEVEL,"sign in successfully\n");
          OspPrintf(1,1,"sign in successfully\n");
          printf("sign in successfully\n");


#if 0

          //测试文件上传
          FILE* file;
          size_t buffer_size;

          if(!(file = fopen(file_name_path,"rb"))){
                  printf("open file error\n");
                  break;
          }

          buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file);
          if(buffer_size > 0){
                post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_UPLOAD
                           ,buffer,buffer_size,GetDstNode());

          }
          //
          while((buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file)) > 0){
                  //TODO:Instance实例选择需要修改,增加中断变量
                post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_UPLOAD
                                ,buffer,buffer_size,m_dwdstNode);
          }
          fclose(file);
#endif


}

void CCInstance::SignOutCmd(CMessage * const pMsg){
          post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_OUT,NULL,0,m_dwdstNode);
          OspLog(SYS_LOG_LEVEL,"get sign out cmd,send to server\n");
          OspPrintf(1,1,"get sign out cmd,send to server\n");

}
void CCInstance::SignOutAck(CMessage * const pMsg){
          NextState(IDLE_STATE);
          OspLog(SYS_LOG_LEVEL,"sign out\n");
          OspPrintf(1,1,"sign out\n");
printf("get sign out ack\n");

}
void CCInstance::FileNameSendCmd(CMessage * const pMsg){

        printf("file name send cmd fun\n");

          if(!pMsg->content || pMsg->length <= 0){
                   OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
                   OspPrintf(1,0,"[InstanceEntry] pMsg is NULL\n");
                   printf("pmsg is null\n");
                   return;
          }
          strcpy((LPSTR)file_name_path,(LPCSTR)pMsg->content);
          post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_NAME_SEND
                         ,pMsg->content,pMsg->length,GetDstNode());

          printf("dstnode:%ld\n",m_dwdstNode);

}
void CCInstance::FileNameAck(CMessage * const pMsg){
          FILE* file;
          size_t buffer_size;


          m_dwDisInsID = pMsg->srcid;
          if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                  printf("open file error\n");
                  return;
          }

#if 0
          buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file);
          if(buffer_size > 0){
                post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_UPLOAD
                           ,buffer,buffer_size,GetDstNode());

          }
#endif

          while((buffer_size = fread(buffer,1,sizeof(s8)*BUFFER_SIZE,file)) > 0){
                  //TODO:增加中断变量,信号量控制,记录文件读取位置
                  //大文件传送测试
                  post(m_dwDisInsID,FILE_UPLOAD,buffer,buffer_size,m_dwdstNode);
                  OspSemTake(m_sem);
                  if(chCancleFlag){
                          OspSemGive(m_sem);
                          break;
                  }
                  OspSemGive(m_sem);
          }
          fclose(file);

}

void CCInstance::MsgProcessInit(){

#if 1
        RegMsgProFun(MAKEESTATE(IDLE_STATE,CLIENT_ENTRY),&CCInstance::ClientEntry,&m_tCmdDaemonChain);

        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_ACK),&CCInstance::SignInAck,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SIGN_OUT_CMD),&CCInstance::SignOutCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SIGN_OUT_ACK),&CCInstance::SignOutAck,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_NAME_SEND_CMD),&CCInstance::FileNameSendCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_NAME_ACK),&CCInstance::FileNameAck,&m_tCmdChain);
#endif
}

void CCInstance::NodeChainEnd(){

        while(m_tCmdChain){
                free(m_tCmdChain);
                m_tCmdChain = m_tCmdChain->next;
        }

        while(m_tCmdDaemonChain){
                free(m_tCmdDaemonChain);
                m_tCmdDaemonChain = m_tCmdDaemonChain->next;
        }
}

bool CCInstance::RegMsgProFun(u32 EventState,MsgProcess c_MsgProcess,tCmdNode** tppNodeChain){

        tCmdNode *Node,*NewNode,*LNode;

        Node = *tppNodeChain;

        if(!(NewNode = (tCmdNode*)malloc(sizeof(tCmdNode)))){
                OspLog(LOG_LVL_ERROR,"[RegMsgProFun] node malloc error\n");
                return false;
        }

        NewNode->EventState = EventState;
        NewNode->c_MsgProcess = c_MsgProcess;
        NewNode->next = NULL;

        if(!Node){
                *tppNodeChain = NewNode;
                OspLog(SYS_LOG_LEVEL,"cmd chain init \n");
                return true;
        }

        while(Node){
                if(Node->EventState == EventState){
                        OspLog(LOG_LVL_ERROR,"[RegMsgProFun] node already in \n");
                        printf("[RegMsgProFun] node already in \n");
                        return false;
                }
                LNode = Node;
                Node = Node->next;
        }
        LNode->next = NewNode;
        return true;
}

bool CCInstance::FindProcess(u32 EventState,MsgProcess* c_MsgProcess,tCmdNode* tNodeChain){

        tCmdNode *Node;

        Node = tNodeChain;
        if(!Node){
                OspLog(LOG_LVL_ERROR,"[FindProcess] Node Chain is NULL\n");
                printf("[FindProcess] Node Chain is NULL\n");
                return false;
        }
        while(Node){
#if 0
                printf("node eventstate:%ld\n",Node->EventState);
                printf("eventstate:%ld\n",EventState);
#endif
                if(Node->EventState == EventState){
                        *c_MsgProcess = Node->c_MsgProcess;
                        return true;
                }
                Node = Node->next;
        }
        return false;
}

#endif

void CCInstance::InstanceEntry(CMessage * const pMsg){

//        printf("client instance entry\n");
        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
        }

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        MsgProcess c_MsgProcess;


        if(FindProcess(MAKEESTATE(curState,curEvent),&c_MsgProcess,m_tCmdChain)){
//                printf("find the EState\n");
                (this->*c_MsgProcess)(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] can not find the EState\n");
                printf("[InstanceEntry] can not find the EState\n");
        }

}

void CCInstance::DaemonInstanceEntry(CMessage *const pMsg,CApp *pCApp){

        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
        }

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        MsgProcess c_MsgProcess;


        if(FindProcess(MAKEESTATE(curState,curEvent),&c_MsgProcess,m_tCmdDaemonChain)){
//                printf("find the EState\n");
                (this->*c_MsgProcess)(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] can not find the EState\n");
                printf("[InstanceEntry] can not find the EState\n");
        }

}
