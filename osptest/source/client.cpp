#include"osp.h"
#include"client.h"

CCApp g_cCApp;
static u32 g_dwdstNode;

#define MAKEESTATE(state,event) ((u32)(event<< 4 +state))
#define SERVER_IP                ("172.16.236.241")
#define SERVER_PORT              (20000)

API void SendSignInCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_IN_CMD);
}


API void SendClientEntryCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),CLIENT_ENTRY);
}

API void SendSignOutCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_OUT_CMD);
}

API void SendFileUploadCmd(){

        s8 file_name[] = "test_file_name";
        ::OspPost(MAKEIID(CLIENT_APP_ID,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        file_name,strlen(file_name)+1);
}

void CCInstance::FileUploadCmd(CMessage*const pMsg){

          if(!pMsg->content || pMsg->length <= 0){
                   OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
                   OspPrintf(1,0,"[InstanceEntry] pMsg is NULL\n");
                   printf("pmsg is null\n");
                   return;
          }

          strcpy((LPSTR)file_name_path,(LPCSTR)pMsg->content);
          if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                  OspLog(LOG_LVL_ERROR,"[SendFileUpload]open file failed\n");
                  printf("[SendFileUpload]open file failed\n");
                  return;
          }

          if(OSP_OK != post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_NAME_SEND
                         ,pMsg->content,pMsg->length,g_dwdstNode)){
                  OspLog(LOG_LVL_ERROR,"[FileUploadCmd] post error\n");
                  return;
          }
          NextState(RUNNING_STATE);
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
        OspRegCommand("ClientEntry",(void*)SendClientEntryCmd,"");
        OspRegCommand("SignIn",(void*)SendSignInCmd,"");
        OspRegCommand("SignOut",(void*)SendSignOutCmd,"");
        OspRegCommand("FileUpload",(void*)SendFileUploadCmd,"");
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

typedef struct tagCmdNode{
        u32         EventState;
        CCInstance::MsgProcess  c_MsgProcess;
        struct      tagCmdNode *next;
}tCmdNode;


void CCInstance::ClientEntry(CMessage *const pMsg){

          if(m_bConnectedFlag){
                  OspLog(SYS_LOG_LEVEL,"connected already\n");
                  return;
          }
          g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
          if(INVALID_NODE == g_dwdstNode){
//                SetTimer(DISCONNET_TIMER, osp_test_interval);
//                OspLog(LOG_LVL_KEY, "Connect extern node failed. exit.\n");
//                OspTaskResume(g_hMainTask);
                  return;
          }
          m_bConnectedFlag = true;
}

void CCInstance::SignInCmd(CMessage *const pMsg){

          if(!m_bConnectedFlag){
                  OspLog(LOG_LVL_ERROR,"[SignInCmd]not connected\n");
          }
          if(m_bSignFlag){
                  OspLog(SYS_LOG_LEVEL,"sign in already\n");
                  return;
          }
          TSinInfo tSinInfo;
          strcpy(tSinInfo.g_Username,"admin");
          strcpy(tSinInfo.g_Passwd,"admin");
          if(post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_IN
                      ,(const void *)&tSinInfo,(u16)sizeof(tSinInfo),g_dwdstNode) != OSP_OK){
                  OspLog(LOG_LVL_ERROR,"[SignInCmd] post error\n");
                  return;
          }
          m_bSignFlag = true;
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
}

void CCInstance::SignOutCmd(CMessage * const pMsg){

          if(OSP_OK != post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON)
                                  ,SIGN_OUT,NULL,0,g_dwdstNode)){
                  OspLog(LOG_LVL_ERROR,"[SignOutCmd] post error\n");
                  return;
          }
          OspLog(SYS_LOG_LEVEL,"get sign out cmd,send to server\n");
          OspPrintf(1,1,"get sign out cmd,send to server\n");

}

void CCInstance::SignOutAck(CMessage * const pMsg){
          NextState(IDLE_STATE);
          OspLog(SYS_LOG_LEVEL,"sign out\n");
          OspPrintf(1,1,"sign out\n");
          printf("get sign out ack\n");

}

void CCInstance::FileNameAck(CMessage * const pMsg){

          size_t buffer_size;

          m_dwDisInsID = pMsg->srcid;
          if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                  printf("open file error\n");
                  return;
          }

          buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file);
          if(buffer_size > 0){
                if(OSP_OK != post(pMsg->srcid,FILE_UPLOAD
                           ,buffer,buffer_size,g_dwdstNode)){
                    OspLog(LOG_LVL_ERROR,"[FileNameAck] post error\n");
                }

          }

}

void CCInstance::FileUploadAck(CMessage* const pMsg){

        size_t buffer_size;

        buffer_size = fread(buffer,1,sizeof(char*)*BUFFER_SIZE,file);
        if(ferror(file)){
                fclose(file);
                OspLog(LOG_LVL_ERROR,"[FileUploadAck] read-file error\n");
                return;
        }
        if(feof(file)){
             if(OSP_OK != post(pMsg->srcid,FILE_FINISH
                           ,buffer,buffer_size,g_dwdstNode)){
                  OspLog(LOG_LVL_ERROR,"[FileUploadAck]FILE_FINISH post error\n");
                  return;
             }
   
        }else{
             if(OSP_OK != post(pMsg->srcid,FILE_UPLOAD
                           ,buffer,buffer_size,g_dwdstNode)){
                  OspLog(LOG_LVL_ERROR,"[FileUploadAck]FILE_UPLOAD post error\n");
                  return;
             }

        }
}

void CCInstance::FileFinishAck(CMessage* const pMsg){
        
        OspLog(SYS_LOG_LEVEL,"file upload finish\n");
        NextState(IDLE_STATE);
        fclose(file);
}

void CCInstance::MsgProcessInit(){

        //Daemon Instance
        RegMsgProFun(MAKEESTATE(IDLE_STATE,CLIENT_ENTRY),&CCInstance::ClientEntry,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_CMD),&CCInstance::SignInCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_ACK),&CCInstance::SignInAck,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SIGN_OUT_CMD),&CCInstance::SignOutCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SIGN_OUT_ACK),&CCInstance::SignOutAck,&m_tCmdDaemonChain);


        //common Instance
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_UPLOAD_CMD),&CCInstance::FileUploadCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_NAME_ACK),&CCInstance::FileNameAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_UPLOAD_ACK),&CCInstance::FileUploadAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_FINISH_ACK),&CCInstance::FileFinishAck,&m_tCmdChain);
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

                if(Node->EventState == EventState){
                        *c_MsgProcess = Node->c_MsgProcess;
                        return true;
                }
                Node = Node->next;
        }

        return false;
}


void CCInstance::InstanceEntry(CMessage * const pMsg){

        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
        }

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        MsgProcess c_MsgProcess;


        if(FindProcess(MAKEESTATE(curState,curEvent),&c_MsgProcess,m_tCmdChain)){
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
                (this->*c_MsgProcess)(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] can not find the EState\n");
                printf("[InstanceEntry] can not find the EState\n");
        }

}
