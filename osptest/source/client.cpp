#include"osp.h"
#include"client.h"

#define MAKEESTATE(state,event) ((u32)((event) << 4 + (state)))
#define SERVER_IP                ("172.16.236.241")
#define SERVER_PORT              (20000)
#define CLIENT_APP_SUM           (5)
#define APP_NUM_SIZE             (20)

static u32 g_dwdstNode;
static CCApp *g_cCApp[CLIENT_APP_SUM];

API void SendFileGoOnCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CLIENT_INSTANCE_ID),FILE_GO_ON_CMD,
                        NULL,0);
}

API void SendCancelCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CLIENT_INSTANCE_ID),SEND_CANCEL_CMD,
                        NULL,0);
}

API void SendRemoveCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CLIENT_INSTANCE_ID),SEND_REMOVE_CMD,
                        NULL,0);
}

API void SendSignInCmd(){

        TSinInfo tSinInfo;
        strcpy(tSinInfo.g_Username,"admin");
        strcpy(tSinInfo.g_Passwd,"admin");

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_IN_CMD,
                        &tSinInfo,sizeof(tSinInfo));
}

API void SendClientEntryCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),CLIENT_ENTRY);
}

API void SendSignOutCmd(){

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_OUT_CMD);
}


API void UploadCmdSingle(const s8* filename){

        u16 i,wAppId;
        bool wPendingFlag = false;
        CCInstance *ccIns;

        for(i = 0;i < CLIENT_APP_SUM;i++){
                ccIns = (CCInstance*)((CApp*)g_cCApp[i])->GetInstance(CLIENT_INSTANCE_ID);
                if(!ccIns){
                        OspLog(LOG_LVL_ERROR,"[UploadCmdSingle] can not find client instance\n");
                        printf("[UploadCmdSingle] can not find client instance\n");
                        return;
                }
                wAppId = ccIns->GetAppID();
                if(ccIns->CurState() == CInstance::PENDING){
                        wPendingFlag = true;
                        break;
                }
        }
        if(!wPendingFlag){
                OspLog(LOG_LVL_ERROR,"[UploadCmdSingle] max file uploaded arrived\n");
                return;
        }
        OspLog(SYS_LOG_LEVEL,"appid:%d\n",wAppId);
        printf("appid:%d\b",wAppId);
        ::OspPost(MAKEIID(wAppId,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        filename,strlen(filename)+1);
}

API void SendFileUploadCmd(){

        UploadCmdSingle("mydoc.7z");
}

API void MultSendFileUploadCmd(){

#if 0
        UploadCmdSingle("mydoc.7z");
        UploadCmdSingle("test_file_name");
#else
        ::OspPost(MAKEIID(3,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        "mydoc.7z",strlen("mydoc.7z")+1);
       ::OspPost(MAKEIID(4,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        "mydoc.7z",strlen("mydoc.7z")+1);
#endif
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
          //获取文件大小，根据标准c，二进制流SEEK_END没有严格得到支持，ftell返回值为long int，
          //32位系统大小限制在2G
          if(fseek(file,0L,SEEK_END) != 0){
                   OspLog(LOG_LVL_ERROR,"[InstanceEntry] file fseeek error\n");
                   return;
          }
          if(-1L == (m_wFileSize = ftell(file))){
                   OspLog(LOG_LVL_ERROR,"[InstanceEntry] file ftell error\n");
                   perror("file ftell error\n");
                   return;
          }
          rewind(file);

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
        s16 i,j;
        s8 chAppNum[APP_NUM_SIZE];

        if(!ret){
                OspPrintf(1,0,"osp init fail\n");
        }

        printf("demo client osp\n");
        for(i = 0;i < CLIENT_APP_SUM;i++ ){
                g_cCApp[i] = new CCApp();
#if 0
                //设计到线程安全，不使用malloc
                g_cCApp[i] = (CCApp*)malloc(sizeof(CCApp));
#endif
                if(!g_cCApp[i]){
                        OspLog(LOG_LVL_ERROR,"[main]app malloc error\n");
                        printf("[main]app malloc error\n");
                        for(j = 0;j < i;j++){
                                delete(g_cCApp[j]);
                        }
                        OspQuit();
                        return -1;
                }
                g_cCApp[i]->CreateApp("OspClientApp"+sprintf(chAppNum,"%d",i)
                                ,CLIENT_APP_ID+i,CLIENT_APP_PRI,MAX_MSG_WAITING);
        }
//        g_cCApp.CreateApp("OspClientApp",CLIENT_APP_ID,CLIENT_APP_PRI,MAX_MSG_WAITING);
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
        OspRegCommand("MFileUpload",(void*)MultSendFileUploadCmd,"");
        OspRegCommand("Cancel",(void*)SendCancelCmd,"");
        OspRegCommand("Remove",(void*)SendRemoveCmd,"");
        OspRegCommand("GoOn",(void*)SendFileGoOnCmd,"");
#endif
        while(1)
                OspDelay(100);

        OspQuit();
        for(i = 0;i < CLIENT_APP_SUM;i++ ){
                delete(g_cCApp[i]);
        }
        return 0;
}

void CCInstance::ClientEntry(CMessage *const pMsg){

          if(m_bConnectedFlag){
                  OspLog(SYS_LOG_LEVEL,"connected already\n");
                  return;
          }
          g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
          if(INVALID_NODE == g_dwdstNode){
                  OspLog(LOG_LVL_KEY, "Connect extern node failed. exit.\n");
                  return;
          }
          m_bConnectedFlag = true;
}

void CCInstance::SignInCmd(CMessage *const pMsg){

          if(!pMsg->content || pMsg->length <= 0){
                  OspLog(LOG_LVL_ERROR,"[SignInCmd] pMsg content is NULL\n");
                  return;
          }
          if(!m_bConnectedFlag){
                  OspLog(LOG_LVL_ERROR,"[SignInCmd]not connected\n");
                  return;
          }
          if(m_bSignFlag){
                  OspLog(SYS_LOG_LEVEL,"sign in already\n");
                  return;
          }
          if(post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_IN
                      ,pMsg->content,pMsg->length,g_dwdstNode) != OSP_OK){
                  OspLog(LOG_LVL_ERROR,"[SignInCmd] post error\n");
                  return;
          }
}

void CCInstance::SignInAck(CMessage * const pMsg){

          if(pMsg->length <= 0 || !pMsg->content){
                  OspLog(SYS_LOG_LEVEL,"sign in failed\n");
                  OspPrintf(1,1,"sign in failed\n");
                  printf("sign in failed\n");
                  return;
                  //TODO:向GUI发送重新登陆提示
          }
          if(strcmp((LPCSTR)pMsg->content,"failed") == 0){
                  OspLog(SYS_LOG_LEVEL,"sign in failed\n");
                  OspPrintf(1,1,"sign in failed\n");
                  printf("sign in failed\n");
                  printf("content:%s\n",pMsg->content);
                  return;
                  //TODO:向GUI发送重新登陆提示
          }
          m_bSignFlag = true;
//          NextState(RUNNING_STATE);
          //TODO,应该需要增加其他信息，目前是无法定位的，也有可能log函数自带定位
          //，需要确认
          OspLog(SYS_LOG_LEVEL,"sign in successfully\n");
          OspPrintf(1,1,"sign in successfully\n");
          printf("sign in successfully\n");
}

void CCInstance::SignOutCmd(CMessage * const pMsg){


          if(!m_bSignFlag){
                  OspLog(SYS_LOG_LEVEL,"haven't sign in\n");
                  return;
          }
          if(OSP_OK != post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON)
                                  ,SIGN_OUT,NULL,0,g_dwdstNode)){
                  OspLog(LOG_LVL_ERROR,"[SignOutCmd] post error\n");
                  return;
          }
          OspLog(SYS_LOG_LEVEL,"get sign out cmd,send to server\n");
          OspPrintf(1,1,"get sign out cmd,send to server\n");

}

void CCInstance::SignOutAck(CMessage * const pMsg){

//          NextState(IDLE_STATE);
          m_bSignFlag = false;
          OspLog(SYS_LOG_LEVEL,"sign out\n");
          printf("get sign out ack\n");

}

void CCInstance::FileNameAck(CMessage * const pMsg){

          size_t buffer_size;

          m_dwDisInsID = pMsg->srcid;

          buffer_size = fread(buffer,1,sizeof(s8)*BUFFER_SIZE,file);
          if(ferror(file)){
                  if(fclose(file) == 0){
                          OspLog(SYS_LOG_LEVEL,"[FileNameAck]file closed\n");

                  }else{
                          OspLog(LOG_LVL_ERROR,"[FileNameAck]file close failed\n");
                  }
                  //TODO:通知server关闭文件
                  OspLog(LOG_LVL_ERROR,"[FileNameAck] read-file error\n");
                  return;
          }
          if(feof(file)){//文件已读取完毕，终止
               if(OSP_OK != post(pMsg->srcid,FILE_FINISH
                             ,buffer,buffer_size,g_dwdstNode)){
                    OspLog(LOG_LVL_ERROR,"[FileNameAck]FILE_FINISH post error\n");
                    return;
               }
          
          }else{
               if(OSP_OK != post(pMsg->srcid,FILE_UPLOAD
                             ,buffer,buffer_size,g_dwdstNode)){
                    OspLog(LOG_LVL_ERROR,"[FileNameAck]FILE_UPLOAD post error\n");
                    return;
               }
          }
          m_wUploadFileSize += buffer_size;
          printf("upload file rate:%f\n",(float)m_wUploadFileSize/(float)m_wFileSize);
}

void CCInstance::FileUploadAck(CMessage* const pMsg){

        size_t buffer_size;

        if(!pMsg->content || pMsg->length <= 0){
                 OspLog(LOG_LVL_ERROR,"[FileUploadAck] pMsg content is NULL\n");
                 return;
        }

        emFileStatus = *((EM_FILE_STATUS*)pMsg->content);
//        memcpy((void*)emFileStatus,(const void*)pMsg->content,pMsg->length);
        if(emFileStatus == GO_ON_SEND){//继续发送
                buffer_size = fread(buffer,1,sizeof(s8)*BUFFER_SIZE,file);
                if(ferror(file)){
                        if(fclose(file) == 0){
                                OspLog(SYS_LOG_LEVEL,"[FileUploadAck]file closed\n");

                        }else{
                                OspLog(LOG_LVL_ERROR,"[FileUploadAck]file close failed\n");
                        }
                        OspLog(LOG_LVL_ERROR,"[FileUploadAck] read-file error\n");
                        return;
                }
                if(feof(file)){//文件已读取完毕，终止
                     if(OSP_OK != post(pMsg->srcid,FILE_FINISH
                                   ,buffer,buffer_size,g_dwdstNode)){
                          OspLog(LOG_LVL_ERROR,"[FileUploadAck]FILE_FINISH post error\n");
                     }
           
                }else{//继续发送
                     if(OSP_OK != post(pMsg->srcid,FILE_UPLOAD
                                   ,buffer,buffer_size,g_dwdstNode)){
                          OspLog(LOG_LVL_ERROR,"[FileUploadAck]FILE_UPLOAD post error\n");
                     }
                }
                m_wUploadFileSize += buffer_size;
                printf("upload file rate:%f\n",(float)m_wUploadFileSize/(float)m_wFileSize);
        }else if(emFileStatus == RECEIVE_CANCEL){//暂停
                if(OSP_OK != post(pMsg->srcid,FILE_CANCEL
                              ,NULL,0,g_dwdstNode)){
                     OspLog(LOG_LVL_ERROR,"[FileUploadAck]FILE_CANCEL post error\n");
                     return;
                }
        }else if(emFileStatus == RECEIVE_REMOVE){//删除文件
                if(OSP_OK != post(pMsg->srcid,FILE_REMOVE
                              ,NULL,0,g_dwdstNode)){
                     OspLog(LOG_LVL_ERROR,"[FileUploadAck]FILE_REMOVE post error\n");
                     return;
                }
        }else{
                OspLog(LOG_LVL_ERROR,"[FileUploadAck]get incorrect file status\n");
                printf("[FileUploadAck]get incorrect file status\n");
        }
}

void CCInstance::FileFinishAck(CMessage* const pMsg){
        
        OspLog(SYS_LOG_LEVEL,"file upload finish\n");
        if(fclose(file) == 0){
                OspLog(SYS_LOG_LEVEL,"[FileRemove]file closed\n");

        }else{
                OspLog(LOG_LVL_ERROR,"[FileRemove]file close failed\n");
        }
        NextState(IDLE_STATE);
        m_wFileSize = 0;
        m_wUploadFileSize = 0;
}

void CCInstance::MsgProcessInit(){

        //Daemon Instance
        RegMsgProFun(MAKEESTATE(IDLE_STATE,CLIENT_ENTRY),&CCInstance::ClientEntry,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_CMD),&CCInstance::SignInCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_ACK),&CCInstance::SignInAck,&m_tCmdDaemonChain);
#if 0
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SIGN_OUT_CMD),&CCInstance::SignOutCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SIGN_OUT_ACK),&CCInstance::SignOutAck,&m_tCmdDaemonChain);
#else
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_OUT_CMD),&CCInstance::SignOutCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_OUT_ACK),&CCInstance::SignOutAck,&m_tCmdDaemonChain);
#endif

        //common Instance
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_UPLOAD_CMD),&CCInstance::FileUploadCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_NAME_ACK),&CCInstance::FileNameAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_UPLOAD_ACK),&CCInstance::FileUploadAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_FINISH_ACK),&CCInstance::FileFinishAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SEND_REMOVE_CMD),&CCInstance::RemoveCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SEND_CANCEL_CMD),&CCInstance::CancelCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_CANCEL_ACK),&CCInstance::FileCancelAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_REMOVE_ACK),&CCInstance::FileRemoveAck,&m_tCmdChain);

        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_GO_ON_CMD),&CCInstance::FileGoOnCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_GO_ON_ACK),&CCInstance::FileGoOnAck,&m_tCmdChain);
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

void CCInstance::RemoveCmd(CMessage* const pMsg){

        if(OSP_OK != post(m_dwDisInsID,SEND_REMOVE
                       ,NULL,0,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                return;
        }
}

void CCInstance::FileGoOnCmd(CMessage* const pMsg){

        if(emFileStatus == FINISHED){
                OspLog(SYS_LOG_LEVEL,"file already finished\n");
                return;
        }
        //暂停或终止处理延迟，重新放入客户端消息队列
        while(emFileStatus < CANCELED){
                if(OSP_OK != post(MAKEIID(CLIENT_APP_ID,CLIENT_INSTANCE_ID),FILE_GO_ON_CMD
                               ,pMsg->content,pMsg->length,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[FileGoOnCmd] post error\n");
                        return;
                }
        }

        if(OSP_OK != post(m_dwDisInsID,FILE_GO_ON
                       ,pMsg->content,pMsg->length,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]file go on post error\n");
                return;
        }
}


void CCInstance::FileGoOnAck(CMessage* const pMsg){

        if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                OspLog(LOG_LVL_ERROR,"[SendFileUpload]open file failed\n");
                printf("[SendFileUpload]open file failed\n");
                return;
        }

        if(fseek(file,m_wUploadFileSize,SEEK_SET) != 0){
                 OspLog(LOG_LVL_ERROR,"[InstanceEntry] file fseeek error\n");
                 return;
        }

        FileNameAck(pMsg);
}

void CCInstance::CancelCmd(CMessage* const pMsg){

        if(OSP_OK != post(m_dwDisInsID,SEND_CANCEL
                       ,NULL,0,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[CancelCmd] post error\n");
                return;
        }
}

void CCInstance::FileCancelAck(CMessage* const pMsg){

        if(fclose(file) == 0){
                OspLog(SYS_LOG_LEVEL,"[FileCancelAck]file closed\n");

        }else{
                OspLog(LOG_LVL_ERROR,"[FileCancelAck]file close failed\n");
        }
        emFileStatus = CANCELED;
}

void CCInstance::FileRemoveAck(CMessage* const pMsg){

        if(fclose(file) == 0){
                OspLog(SYS_LOG_LEVEL,"[FileCancelAck]file closed\n");

        }else{
                OspLog(LOG_LVL_ERROR,"[FileCancelAck]file close failed\n");
        }
        emFileStatus = REMOVED;
        m_wUploadFileSize = 0;
        m_wFileSize = 0;
        NextState(IDLE_STATE);
}


