#include"osp.h"
#include"client.h"

#define MAKEESTATE(state,event) ((u32)((event) << 4 + (state)))
#define CLIENT_APP_SUM           (5)
#define APP_NUM_SIZE             (20)

#define SERVER_DELAY             (1000)

#if 0
#define MAX_CMD_REPEAT_TIMES     5
#endif

API void Connect2Server();
API void SendSignInCmd();
API void SendSignOutCmd();
API void SendFileUploadCmd();
API void MultSendFileUploadCmd();
API void SendCancelCmd();
API void SendRemoveCmd();
API void SendFileGoOnCmd();

static void UploadCmdSingle(const s8*);

static u32 g_dwdstNode;
static bool g_bConnectedFlag;
static bool g_bSignFlag;

static CCApp *g_cCApp[CLIENT_APP_SUM];

static u16 g_wTestSingleAppId;

int main(){

#ifdef _MSC_VER
        int ret = OspInit(TRUE,2501,"WindowsOspClient");
#else
        int ret = OspInit(TRUE,2501,"LinuxOspClient");
#endif
        s16 i,j;
        s8 chAppNum[APP_NUM_SIZE];
        g_bConnectedFlag = false;
        g_bSignFlag = false;

        if(!ret){
                OspPrintf(1,0,"osp init fail\n");
        }

        printf("demo client osp\n");
        for(i = 0;i < CLIENT_APP_SUM;i++ ){
                g_cCApp[i] = new CCApp();
#if 0
                //涉及到线程安全，不使用malloc
                g_cCApp[i] = (CCApp*)malloc(sizeof(CCApp));
#endif
                if(!g_cCApp[i]){
                        OspLog(LOG_LVL_ERROR,"[main]app malloc error\n");
                        printf("[main]app malloc error\n");
                        OspQuit();
                        for(j = 0;j < i;j++){
                                delete(g_cCApp[j]);
                        }
                        return -1;
                }
                g_cCApp[i]->CreateApp("OspClientApp"+sprintf(chAppNum,"%d",i)
                                ,CLIENT_APP_ID+i,CLIENT_APP_PRI,MAX_MSG_WAITING);
        }
        ret = OspCreateTcpNode(0,OSP_AGENT_CLIENT_PORT);
        if(INVALID_SOCKET == ret){
                OspQuit();
                for(i = 0;i < CLIENT_APP_SUM;i++ ){
                        delete(g_cCApp[i]);
                }
                OspPrintf(1,0,"[main]create positive node failed,quit\n");
                printf("[main]node created failed\n");
                return -1;
        }

#ifdef _LINUX_
        OspRegCommand("Connect",(void*)Connect2Server,"");
        OspRegCommand("SignIn",(void*)SendSignInCmd,"");
        OspRegCommand("SignOut",(void*)SendSignOutCmd,"");
        OspRegCommand("FileUpload",(void*)SendFileUploadCmd,"");
        OspRegCommand("MFileUpload",(void*)MultSendFileUploadCmd,"");
        OspRegCommand("Cancel",(void*)SendCancelCmd,"");
        OspRegCommand("Remove",(void*)SendRemoveCmd,"");
        OspRegCommand("GoOn",(void*)SendFileGoOnCmd,"");
#endif
        //TODO:连接参数可选配置
        g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
        if(INVALID_NODE == g_dwdstNode){
                OspLog(SYS_LOG_LEVEL, "[main]Connect to server faild.\n");
#if 0
                //尝试再次连接由sign in 执行
                //TODO:提供连接参数输入，尝试再次连接
                scanf("ip:%d port:%d\n",server_ip,server_port);
                g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
                if(INVALID_NODE == g_dwdstNode){
                        OspLog(SYS_LOG_LEVEL, "[main]Connect to server faild again.\n");
                }
#endif
        }else{
                OspLog(SYS_LOG_LEVEL, "[main]Connect to server successfully.\n");
                g_bConnectedFlag = true;
        }
        while(1)
                OspDelay(100);

        OspQuit();
        for(i = 0;i < CLIENT_APP_SUM;i++ ){
                delete(g_cCApp[i]);
        }
        return 0;
}

API void Connect2Server(){

          if(g_bConnectedFlag){
                  OspLog(SYS_LOG_LEVEL,"connected already\n");
                  return;
          }
          g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
          if(INVALID_NODE == g_dwdstNode){
                  OspLog(LOG_LVL_KEY, "Connect extern node failed. exit.\n");
                  return;
          }
          g_bConnectedFlag = true;
}

API void SendFileGoOnCmd(){


        if(OSP_OK != ::OspPost(MAKEIID(g_wTestSingleAppId,CLIENT_INSTANCE_ID),FILE_GO_ON_CMD,
                        NULL,0)){
               OspLog(LOG_LVL_ERROR,"[SendFileGoOnCmd] post error\n");
        }
}

API void SendCancelCmd(){


       if(OSP_OK != ::OspPost(MAKEIID(g_wTestSingleAppId,CLIENT_INSTANCE_ID),SEND_CANCEL_CMD,
                        NULL,0)){
               OspLog(LOG_LVL_ERROR,"[SendCancel] post error\n");
       }
}

API void SendRemoveCmd(){

        if(OSP_OK != ::OspPost(MAKEIID(g_wTestSingleAppId,CLIENT_INSTANCE_ID),SEND_REMOVE_CMD,
                        NULL,0)){
               OspLog(LOG_LVL_ERROR,"[SendRemoveCmd] post error\n");
        }
}

API void SendSignInCmd(){

        TSinInfo tSinInfo;

        //查询所有app instance，找到空闲的instance来执行sign in操作
        u16 i,wAppId;
        bool bPendingFlag = false;
        CCInstance *ccIns;

        strcpy(tSinInfo.Username,"admin");
        strcpy(tSinInfo.Passwd,"admin");

        for(i = 0;i < CLIENT_APP_SUM;i++){
                ccIns = (CCInstance*)((CApp*)g_cCApp[i])->GetInstance(CLIENT_INSTANCE_ID);
                if(!ccIns){
                        OspLog(LOG_LVL_ERROR,"[SendSignInCmd] can not find client instance\n");
                        printf("[SendSignInCmd] can not find client instance\n");
                        return;
                }
                wAppId = ccIns->GetAppID();
                if(ccIns->CurState() == CInstance::PENDING){
                        bPendingFlag = true;
                        break;
                }
        }
        if(!bPendingFlag){
                OspLog(LOG_LVL_ERROR,"[SendSignInCmd] max file uploaded arrived\n");
                return;
        }

        if(OSP_OK != ::OspPost(MAKEIID(wAppId,CInstance::DAEMON),SIGN_IN_CMD,
                        &tSinInfo,sizeof(tSinInfo))){
               OspLog(LOG_LVL_ERROR,"[SendSignInCmd] post error\n");
        }
}


API void SendSignOutCmd(){

        if(OSP_OK != ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_OUT_CMD)){
               OspLog(LOG_LVL_ERROR,"[SendSignOutCmd] post error\n");
        }
}

static CCInstance* GetPendingInstance(u16 insId,CCApp *ccApp[],u16 appNum){

        u16 i;
        bool bPendingFlag = false;
        CCInstance *ccIns;

        for(i = 0;i < appNum;i++){
                ccIns = (CCInstance*)((CApp*)ccApp[i])->GetInstance(insId);
                if(!ccIns){
                        return NULL;
                }
                if(ccIns->CurState() == CInstance::PENDING){
                        bPendingFlag = true;
                        break;
                }
        }
        if(!bPendingFlag){
                return NULL;
        }
        return ccIns;
}

static void UploadCmdSingle(const s8* filename){

        CCInstance *ccIns;

        ccIns = GetPendingInstance(CLIENT_INSTANCE_ID,g_cCApp,CLIENT_APP_SUM);
        if(!ccIns){
                OspLog(LOG_LVL_ERROR,"[UploadCmdSingle] can not find client instance\n");
                printf("[UploadCmdSingle] can not find client instance\n");
                return;
        }
        g_wTestSingleAppId = ccIns->GetAppID();
        OspPrintf(1,0,"appid:%d\n",g_wTestSingleAppId);
        if(OSP_OK !=::OspPost(MAKEIID(ccIns->GetAppID(),CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        filename,strlen(filename)+1)){
               OspLog(LOG_LVL_ERROR,"[UploadCmdSingle] post error\n");
        }
}

API void SendFileUploadCmd(){

        UploadCmdSingle("mydoc.7z");
}

API void MultSendFileUploadCmd(){

#if 1
        UploadCmdSingle("mydoc.7z");
        OspDelay(200);
        UploadCmdSingle("test_file_name");
#else
        ::OspPost(MAKEIID(3,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        "mydoc.7z",strlen("mydoc.7z")+1);
       ::OspPost(MAKEIID(4,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        "mydoc.7z",strlen("mydoc.7z")+1);
#endif
}

void CCInstance::FileUploadCmd(CMessage*const pMsg){

        if(!g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[FileUploadCmd]not signed,please sign in first\n");
                //TODO:注册函数改为有返回值的，重新sign in部分改为调用执行
                return;
        }
        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SEND_REMOVE_CMD
                               ,NULL,0,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[FileUploadCmd] post error\n");
                }
                return;
        }

        if(emFileStatus == STATUS_UPLOADING){
                OspLog(SYS_LOG_LEVEL, "[FileUploadCmd]Uploading...\n");
                return;
        }

        if(emFileStatus == STATUS_CANCELLED){
                OspLog(SYS_LOG_LEVEL, "[FileUploadCmd]File Cancelled,please use GoOn\n");
                return;
        }

        if(!pMsg->content || pMsg->length <= 0){
                 OspLog(LOG_LVL_ERROR,"[FileUploadCmd] pMsg is NULL\n");
                 OspPrintf(1,0,"[FileUploadCmd] pMsg is NULL\n");
                 printf("pmsg is null\n");
                 return;
        }

        strcpy((LPSTR)file_name_path,(LPCSTR)pMsg->content);
        
        if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmd]open file failed\n");
                printf("[FileUploadCmd]open file failed\n");
                return;
        }
        //获取文件大小，根据标准c，二进制流SEEK_END没有严格得到支持，ftell返回值为long int，
        //32位系统大小限制在2G
        if(fseek(file,0L,SEEK_END) != 0){
                 OspLog(LOG_LVL_ERROR,"[FileUploadCmd] file fseeek error\n");
                 return;
        }
        if(-1L == (m_wFileSize = ftell(file))){
                 OspLog(LOG_LVL_ERROR,"[FileUploadCmd] file ftell error\n");
                 perror("file ftell error\n");
                 return;
        }
        rewind(file);

        if(OSP_OK != post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_SEND_UPLOAD
                       ,pMsg->content,pMsg->length,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmd] post error\n");
                return;
        }
        NextState(RUNNING_STATE);
        emFileStatus = STATUS_SEND_UPLOAD;
}

void CCInstance::SignInCmd(CMessage *const pMsg){

        u8 server_ip[MAX_IP_LENGTH];
        u16 server_port;

        if(!g_bConnectedFlag){
                //TODO:注册函数改为有返回值的，重新连接部分改为调用执行
                scanf("ip:%s port:%d\n",server_ip,server_port);
                g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
                if(INVALID_NODE == g_dwdstNode){
                        OspLog(LOG_LVL_ERROR, "[SignInCmd]Connect to server faild.\n");
                        return;
                }else{
                        g_bConnectedFlag = true;
                }
        }

        if(!pMsg->content || pMsg->length <= 0){
                OspLog(LOG_LVL_ERROR,"[SignInCmd] pMsg content is NULL\n");
                return;
        }
        if(!g_bConnectedFlag){
                OspLog(LOG_LVL_ERROR,"[SignInCmd]not connected\n");
                return;
        }
        if(g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[SignInCmd]sign in already\n");
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
          g_bSignFlag = true;
          OspLog(SYS_LOG_LEVEL,"[SignInAck]sign in successfully\n");
          printf("[SignInAck]sign in successfully\n");
}

void CCInstance::SignOutCmd(CMessage * const pMsg){

          if(!g_bSignFlag){
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

          g_bSignFlag = false;
          OspLog(SYS_LOG_LEVEL,"[SignOutAck]sign out\n");
          printf("[SignOutAck]get sign out ack\n");
}

void CCInstance::FileReceiveUploadAck(CMessage * const pMsg){

          size_t buffer_size;

          m_dwDisInsID = pMsg->srcid;

          buffer_size = fread(buffer,1,sizeof(s8)*BUFFER_SIZE,file);
          if(ferror(file)){
                  if(fclose(file) == 0){
                          OspLog(SYS_LOG_LEVEL,"[FileReceiveUploadAck]file closed\n");

                  }else{
                          OspLog(LOG_LVL_ERROR,"[FileReceiveUploadAck]file close failed\n");
                  }
                  //TODO:通知server关闭文件
                  OspLog(LOG_LVL_ERROR,"[FileReceiveUploadAck] read-file error\n");
                  return;
          }
          if(feof(file)){//文件已读取完毕，终止
               if(OSP_OK != post(pMsg->srcid,FILE_FINISH
                             ,buffer,buffer_size,g_dwdstNode)){
                    OspLog(LOG_LVL_ERROR,"[FileReceiveUploadAck]FILE_FINISH post error\n");
                    return;
               }
          
          }else{
               if(OSP_OK != post(pMsg->srcid,FILE_UPLOAD
                             ,buffer,buffer_size,g_dwdstNode)){
                    OspLog(LOG_LVL_ERROR,"[FileReceiveUploadAck]FILE_UPLOAD post error\n");
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
        if(emFileStatus == STATUS_UPLOADING){//继续发送
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
        }else if(emFileStatus == STATUS_RECEIVE_CANCEL){//暂停
                if(OSP_OK != post(pMsg->srcid,FILE_CANCEL
                              ,NULL,0,g_dwdstNode)){
                     OspLog(LOG_LVL_ERROR,"[FileUploadAck]FILE_CANCEL post error\n");
                     return;
                }
        }else if(emFileStatus == STATUS_RECEIVE_REMOVE){//删除文件
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
                OspLog(SYS_LOG_LEVEL,"[FileFinishAck]file closed\n");

        }else{
                OspLog(LOG_LVL_ERROR,"[FileFinishAck]file close failed\n");
        }
        emFileStatus = STATUS_FINISHED;
        NextState(IDLE_STATE);
        m_wFileSize = 0;
        m_wUploadFileSize = 0;
}

void CCInstance::MsgProcessInit(){

        //Daemon Instance
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_CMD),&CCInstance::SignInCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_OUT_CMD),&CCInstance::SignOutCmd,&m_tCmdDaemonChain);

        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_OUT_ACK),&CCInstance::SignOutAck,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_ACK),&CCInstance::SignInAck,&m_tCmdDaemonChain);

        //common Instance
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_UPLOAD_CMD),&CCInstance::FileUploadCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SEND_REMOVE_CMD),&CCInstance::RemoveCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SEND_REMOVE_CMD),&CCInstance::RemoveCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SEND_CANCEL_CMD),&CCInstance::CancelCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_GO_ON_CMD),&CCInstance::FileGoOnCmd,&m_tCmdChain);

        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_UPLOAD_ACK),&CCInstance::FileUploadAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_FINISH_ACK),&CCInstance::FileFinishAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_CANCEL_ACK),&CCInstance::FileCancelAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_REMOVE_ACK),&CCInstance::FileRemoveAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_GO_ON_ACK),&CCInstance::FileGoOnAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_RECEIVE_UPLOAD_ACK),&CCInstance::FileReceiveUploadAck
                        ,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_STABLE_REMOVE_ACK),&CCInstance::FileStableRemoveAck,
                        &m_tCmdChain);

        //直接返回不处理，调试信息完整，避免只返回can not find the EState
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SEND_CANCEL_CMD),&CCInstance::CancelCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_GO_ON_CMD),&CCInstance::FileGoOnCmd,&m_tCmdChain);

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
                OspLog(LOG_LVL_ERROR,"[DaemonInstanceEntry] pMsg is NULL\n");
        }

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        MsgProcess c_MsgProcess;


        if(FindProcess(MAKEESTATE(curState,curEvent),&c_MsgProcess,m_tCmdDaemonChain)){
                (this->*c_MsgProcess)(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[DaemonInstanceEntry] can not find the EState\n");
                printf("[DaemonInstanceEntry] can not find the EState\n");
        }
}

void CCInstance::RemoveCmd(CMessage* const pMsg){

        //从GUI的使用来说，这个可以保证
        if(emFileStatus == STATUS_INIT){
                OspLog(LOG_LVL_ERROR,"[RemoveCmd]status error\n");
                return;
        }
        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SEND_REMOVE_CMD
                               ,NULL,0,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                }
                return;
        }

        if(emFileStatus == STATUS_REMOVED){
                OspLog(SYS_LOG_LEVEL,"[RemoveCmd]Already removed\n");
                return;
        }

        if(emFileStatus == STATUS_CANCELLED || emFileStatus == STATUS_FINISHED){
                if(OSP_OK != post(m_dwDisInsID,FILE_STABLE_REMOVE
                               ,NULL,0,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                }
                return;
        }

        if(OSP_OK != post(m_dwDisInsID,SEND_REMOVE
                       ,NULL,0,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                return;
        }
        emFileStatus = STATUS_SEND_REMOVE;
}

void CCInstance::FileGoOnCmd(CMessage* const pMsg){

        //从GUI的使用来说，这个可以保证
        if(emFileStatus == STATUS_INIT){
                OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]status error\n");
                return;
        }

        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),FILE_GO_ON_CMD
                               ,NULL,0,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                }
                return;
        }

        if(emFileStatus == STATUS_FINISHED 
                        || emFileStatus == STATUS_REMOVED){
                OspLog(SYS_LOG_LEVEL,"file already finished or removed\n");
                return;
        }

        if(emFileStatus == STATUS_UPLOADING){
                OspLog(SYS_LOG_LEVEL,"Already go on\n");
                return;
        }

#if 0
        //暂停或终止处理延迟，重新放入客户端消息队列
        //TODO:对于removed的处理可能需要修改
        if(emFileStatus < STATUS_CANCELLED){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),FILE_GO_ON_CMD
                               ,NULL,0,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[FileGoOnCmd] post error\n");
                }
                return;
        }
#endif

        if(OSP_OK != post(m_dwDisInsID,FILE_GO_ON
                       ,NULL,0,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]file go on post error\n");
                return;
        }
        emFileStatus = STATUS_SEND_UPLOAD;
}

void CCInstance::FileGoOnAck(CMessage* const pMsg){

        if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                OspLog(LOG_LVL_ERROR,"[FileGoOnAck]open file failed\n");
                printf("[FileGoOnAck]open file failed\n");
                return;
        }

        if(fseek(file,m_wUploadFileSize,SEEK_SET) != 0){
                 OspLog(LOG_LVL_ERROR,"[FileGoOnAck] file fseeek error\n");
                 return;
        }

        FileReceiveUploadAck(pMsg);
}

void CCInstance::CancelCmd(CMessage* const pMsg){

        //从GUI的使用来说，这个可以保证
        if(emFileStatus == STATUS_INIT){
                OspLog(LOG_LVL_ERROR,"[CancelCmd]status error\n");
                return;
        }

        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SEND_CANCEL_CMD
                               ,NULL,0,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[CancelCmd] post error\n");
                }
                return;
        }

        if(emFileStatus >= STATUS_CANCELLED){
                OspLog(SYS_LOG_LEVEL,"[CancelCmd]Already stable state\n");
                return;
                
        }

        if(OSP_OK != post(m_dwDisInsID,SEND_CANCEL
                       ,NULL,0,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[CancelCmd] post error\n");
                return;
        }
        emFileStatus = STATUS_SEND_CANCEL;
}

void CCInstance::FileCancelAck(CMessage* const pMsg){

        u16 wAppId;

        if(fclose(file) == 0){
                OspLog(SYS_LOG_LEVEL,"[FileCancelAck]file closed\n");

        }else{
                OspLog(LOG_LVL_ERROR,"[FileCancelAck]file close failed\n");
        }
        emFileStatus = STATUS_CANCELLED;
}

void CCInstance::FileRemoveAck(CMessage* const pMsg){

        if(fclose(file) == 0){
                OspLog(SYS_LOG_LEVEL,"[FileRemovelAck]file closed\n");

        }else{
                OspLog(LOG_LVL_ERROR,"[FileRemoveAck]file close failed\n");
        }
        emFileStatus = STATUS_REMOVED;
        m_wUploadFileSize = 0;
        m_wFileSize = 0;
        NextState(IDLE_STATE);
        //TODO:key print
        printf("file remove\n");
}

void CCInstance::FileStableRemoveAck(CMessage* const pMsg){

        emFileStatus = STATUS_REMOVED;
        m_wUploadFileSize = 0;
        m_wFileSize = 0;
        NextState(IDLE_STATE);
        //TODO:key print
        printf("file remove\n");
}
