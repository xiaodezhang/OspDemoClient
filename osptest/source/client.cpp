#include"osp.h"
#include"client.h"
#include"list.h"

#define MAKEESTATE(state,event) ((u32)((event) << 4 + (state)))
#define CLIENT_APP_SUM           5
#define APP_NUM_SIZE             20

#define SERVER_DELAY             1000
#define CREATE_TCP_NODE_TIMES     20
#define MY_FILE_NAME             "mydoc.7z"
#define NATIVE_IP                "127.0.0.1"

#if 0
#define MAX_CMD_REPEAT_TIMES     5
#endif

API void Test_DisConnect();
API void Test_Cancel();
API void Connect2Server();
#if MULTY_APP
API int SendSignInCmd();
#else
API void SendSignInCmd();
#endif
API int SendSignOutCmd();
API void SendFileUploadCmd();
API void MultSendFileUploadCmd();
API void SendCancelCmd();
API void SendRemoveCmd();
API void SendFileGoOnCmd();
API void Disconnect2Server();

static void UploadCmdSingle(const s8*);

static u32 g_dwdstNode;
static u32 g_dwGuiNode;
#if SINGLE_APP
bool        g_bConnectedFlag;    
bool        g_bSignFlag;         
#endif


#if MULTY_APP
static CCApp *g_cCApp[CLIENT_APP_SUM];
#else
static CCApp g_cCApp;
#endif

static u16 g_wTestSingleAppId;

struct list_head tFileList;   //文件表

typedef struct tagFileList{
        struct list_head       tListHead;
        u8                     FileName[MAX_FILE_NAME_LENGTH];
        EM_FILE_STATUS         FileStatus;
        u16                    DealInstance;
        u32                    UploadFileSize;
        u32                    FileSize;
}TFileList;

typedef struct tagDemoInfo{
        u32                    srcid;
        s8                     FileName[MAX_FILE_NAME_LENGTH];
        u32                    UploadFileSize;
        u32                    FileSize;
        EM_FILE_STATUS         emFileStatus;
}TDemoInfo;


static bool CheckFileIn(LPCSTR filename,TFileList **tFile);
static CCInstance* GetPendingIns();

int main(){

#ifdef _MSC_VER
        int ret = OspInit(TRUE,2500,"WindowsOspClient");
#else
        int ret = OspInit(TRUE,2500,"LinuxOspClient");
#endif
        s16 i,j;
        s8 chAppNum[APP_NUM_SIZE];

#if SINGLE_APP
        g_bConnectedFlag = false;
        g_bSignFlag      = false;
#endif
        u32 guiPort;

        if(!ret){
                OspPrintf(1,0,"osp init fail\n");
                return -1;
        }

        printf("demo client osp\n");
#if MULTY_APP
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
                if(OSP_OK != g_cCApp[i]->CreateApp("OspClientApp"+sprintf(chAppNum,"%d",i)
                                ,CLIENT_APP_ID+i,CLIENT_APP_PRI,MAX_MSG_WAITING)){

                        OspLog(LOG_LVL_ERROR,"[main]app create error\n");
                        return -1;
                }
        }
#else
        if(OSP_OK != g_cCApp.CreateApp("OspClientApp",CLIENT_APP_ID,CLIENT_APP_PRI,MAX_MSG_WAITING)){
                OspLog(LOG_LVL_ERROR,"[main]app create error\n");
                return -1;
        }
#endif


#ifdef _LINUX_
        OspRegCommand("tcancel",(void*)Test_Cancel,"");
        OspRegCommand("tdisconnect",(void*)Test_DisConnect,"");
        OspRegCommand("Connect",(void*)Connect2Server,"");
        OspRegCommand("DisConnect",(void*)Disconnect2Server,"");
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
#if MULTY_APP
                for(i = 0;i < CLIENT_APP_SUM;i++){
                        //断链注册
                        if(OSP_OK !=OspNodeDiscCBReg(g_dwdstNode,i+CLIENT_APP_ID,CInstance::DAEMON)){
                            OspLog(LOG_LVL_ERROR,"[main]regis disconnect error\n");
                            return -1;
                        }
                        //通知连接
                        if(OSP_OK != ::OspPost(MAKEIID(i+CLIENT_APP_ID,CInstance::DAEMON),MY_CONNECT,
                                        NULL,0)){
                               OspLog(LOG_LVL_ERROR,"[Connecte2Server] post error\n");
                               //TODO:错误处理方式
                               return -1;
                        }
                }
#else
                //断链注册
                if(OSP_OK !=OspNodeDiscCBReg(g_dwdstNode,CLIENT_APP_ID,CInstance::DAEMON)){
                    OspLog(LOG_LVL_ERROR,"[main]regis disconnect error\n");
                    return -1;
                }

                g_bConnectedFlag = true;
#endif
        }
        guiPort = DemoGuiInit();
        if(guiPort < 0){
                OspLog(LOG_LVL_ERROR,"[main]create native gui node failed\n");
                return -1;
        }

        g_dwGuiNode = OspConnectTcpNode(inet_addr(NATIVE_IP),guiPort,10,3);
        if(OSP_OK != g_dwGuiNode){
                OspLog(LOG_LVL_ERROR,"[main]connect to native gui node failed\n");
                return -1;
        }

        //文件表初始化
        INIT_LIST_HEAD(&tFileList);

        while(1)
                OspDelay(100);

        OspQuit();
#if MULTY_APP
        for(i = 0;i < CLIENT_APP_SUM;i++ ){
                delete(g_cCApp[i]);
        }
#endif
        return 0;
}


API void Test_Cancel(){

        Connect2Server();
        OspDelay(500);
        SendSignInCmd();
        OspDelay(500);
        SendFileUploadCmd();
        SendCancelCmd();
}

API void Test_DisConnect(){

        Connect2Server();
        OspDelay(500);
        SendSignInCmd();
        OspDelay(500);
        SendFileUploadCmd();
        OspDelay(9500);
        Disconnect2Server();
        OspDelay(500);
        Connect2Server();
        OspDelay(500);
        SendSignInCmd();
        OspDelay(500);
        SendFileUploadCmd();
}

API void Test_Sign(){

#if 0
        assert(0 == SendSignInCmd());
        assert(0 == SendSignOutCmd());

        SendSignInCmd();
        SendSignInCmd();
#endif
}

API void Connect2Server(){

        u16 i;

        g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
        if(INVALID_NODE == g_dwdstNode){
                OspLog(LOG_LVL_KEY, "Connect extern node failed. exit.\n");
                return;
        }

#if MULTY_APP
        //断链注册
        for(i = 0;i < CLIENT_APP_SUM;i++){
                //断链注册
                if(OSP_OK !=OspNodeDiscCBReg(g_dwdstNode,i+CLIENT_APP_ID,CInstance::DAEMON)){
                    OspLog(LOG_LVL_ERROR,"[main]regis disconnect error\n");
                    return;
                }
                //通知连接
                if(OSP_OK != ::OspPost(MAKEIID(i+CLIENT_APP_ID,CInstance::DAEMON),MY_CONNECT,
                                NULL,0)){
                       OspLog(LOG_LVL_ERROR,"[Connecte2Server] post error\n");
                       //TODO:错误处理方式
                    return;
                }
        }
#else
        //断链注册
        if(OSP_OK !=OspNodeDiscCBReg(g_dwdstNode,CLIENT_APP_ID,CInstance::DAEMON)){
            OspLog(LOG_LVL_ERROR,"[main]regis disconnect error\n");
            return;
        }

        g_bConnectedFlag = true;
#endif

}

API void Disconnect2Server(){

        if(!OspDisconnectTcpNode(g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[Disconnect2Server]disconnecte failed\n");
                return;
        }
}

API void SendFileGoOnCmd(){

       s8 file_name[] = MY_FILE_NAME;

       if(OSP_OK != ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),FILE_GO_ON_CMD,
                               file_name,strlen(file_name)+1)){
               OspLog(LOG_LVL_ERROR,"[SendCancel] post error\n");
       }

}

API void SendCancelCmd(){

       s8 file_name[] = MY_FILE_NAME;

       if(OSP_OK != ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SEND_CANCEL_CMD,
                               file_name,strlen(file_name)+1)){
               OspLog(LOG_LVL_ERROR,"[SendCancel] post error\n");
       }
}

API void SendRemoveCmd(){

       s8 file_name[] = MY_FILE_NAME;

       if(OSP_OK != ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SEND_REMOVE_CMD,
                       file_name,strlen(file_name)+1)){
              OspLog(LOG_LVL_ERROR,"[SendRemoveCmd] post error\n");
       }
}

#if MULTY_APP
API int SendSignInCmd(){

        TSinInfo tSinInfo;

        //查询所有app instance，找到空闲的instance来执行sign in操作
        u16 i,wAppId;
        bool bPendingFlag = false;
        CCInstance *ccIns;

        strcpy(tSinInfo.Username,"Robert");
        strcpy(tSinInfo.Passwd,"admin");

        for(i = 0;i < CLIENT_APP_SUM;i++){
                ccIns = (CCInstance*)((CApp*)g_cCApp[i])->GetInstance(CLIENT_INSTANCE_ID);
                if(!ccIns){
                        OspLog(LOG_LVL_ERROR,"[SendSignInCmd] can not find client instance\n");
                        printf("[SendSignInCmd] can not find client instance\n");
                        return -1;
                }
                wAppId = ccIns->GetAppID();
                if(ccIns->CurState() == CInstance::PENDING){
                        bPendingFlag = true;
                        break;
                }
        }
        if(!bPendingFlag){
                OspLog(LOG_LVL_ERROR,"[SendSignInCmd] max file uploaded arrived\n");
                return -2;
        }

        if(OSP_OK != ::OspPost(MAKEIID(wAppId,CInstance::DAEMON),SIGN_IN_CMD,
                        &tSinInfo,sizeof(tSinInfo))){
               OspLog(LOG_LVL_ERROR,"[SendSignInCmd] post error\n");
               return -3;
        }
        return 0;
}

#else
API void SendSignInCmd(){

        TSinInfo tSinInfo;

        strcpy(tSinInfo.Username,"Robert");
        strcpy(tSinInfo.Passwd,"admin");

        if(OSP_OK != ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_IN_CMD,
                        &tSinInfo,sizeof(tSinInfo))){
               OspLog(LOG_LVL_ERROR,"[SendSignInCmd] post error\n");
               return;
        }

}
#endif


API int SendSignOutCmd(){

        if(OSP_OK != ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),SIGN_OUT_CMD)){
               OspLog(LOG_LVL_ERROR,"[SendSignOutCmd] post error\n");
        }
}

#if MULTY_APP
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
#else

static void UploadCmdSingle(const s8* filename){

        if(OSP_OK !=::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::DAEMON),FILE_UPLOAD_CMD,
                        filename,strlen(filename)+1)){
               OspLog(LOG_LVL_ERROR,"[UploadCmdSingle] post error\n");
        }
}

#endif

API void SendFileUploadCmd(){

        UploadCmdSingle(MY_FILE_NAME);
}

API void MultSendFileUploadCmd(){

#if 1
        UploadCmdSingle(MY_FILE_NAME);
        OspDelay(200);
        UploadCmdSingle("test_file_name");
#else
        ::OspPost(MAKEIID(3,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        MY_FILE_NAME,strlen("mydoc.7z")+1);
       ::OspPost(MAKEIID(4,CLIENT_INSTANCE_ID),FILE_UPLOAD_CMD,
                        MY_FILE_NAME,strlen("mydoc.7z")+1);
#endif
}

void CCInstance::FileUploadCmd(CMessage*const pMsg){

        CCInstance *ccIns;
        TFileList *tnFile = NULL;
#if 0
        if(!m_bConnectedFlag){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmd]disconnected\n");
                return;
        }
#endif

        //多app：
        //连接和登陆状态由广播获得，若间隔太小则可能发生错误，调用的时候需要确保
#if MULTY_APP
        if(!m_bSignFlag){
#else
        if(!g_bSignFlag){
#endif
                OspLog(SYS_LOG_LEVEL,"[FileUploadCmd]did not sign in\n");
                return;
        }

        if(!pMsg->content || pMsg->length <= 0){
                 OspLog(LOG_LVL_ERROR,"[FileUploadCmd] pMsg is NULL\n");
                 printf("[FileUploadCmd]pmsg is null\n");
                 return;
        }


        //确认文件没有被其他Instance占用
        //TODO: 其他状态的确认
        if(CheckFileIn((LPCSTR)pMsg->content,&tnFile) && STATUS_FINISHED != tnFile->FileStatus
                        && STATUS_REMOVED != tnFile->FileStatus){
                OspLog(SYS_LOG_LEVEL,"[FileUploadCmd]file is not finished or removed\n");
                return;
        }
         //查询空闲Instance
        if(!(ccIns = GetPendingIns())){
                OspLog(SYS_LOG_LEVEL, "[FileUploadCmd]no pending instance,please wait...\n");
                return;
        }

        if(OSP_OK != post(MAKEIID(CLIENT_APP_ID,ccIns->GetInsID()),FILE_UPLOAD_CMD_DEAL,
                        pMsg->content,pMsg->length)){
               OspLog(LOG_LVL_ERROR,"[FileUploadCmd] post error\n");
               return;
        }
        //立刻将该Instance状态设置为RUNNING，防止因为立刻调用其他处理导致该instance
        //被其他任务(业务)查询之后调用
        ccIns->m_curState = RUNNING_STATE;

        if(!tnFile){
            tnFile = new TFileList();
            if(!tnFile){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmd]file list item malloc failed\n");
                //TODO：状态回收
                return;
            }
            list_add(&tnFile->tListHead,&tFileList);
        }
        strcpy((LPSTR)tnFile->FileName,(LPSTR)pMsg->content);
        tnFile->DealInstance = ccIns->GetInsID();
        tnFile->FileStatus = STATUS_UPLOAD_CMD;



#if MULTY_APP
        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SEND_REMOVE_CMD
                               ,NULL,0)){
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

        if(OSP_OK != post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),FILE_SEND_UPLOAD
                       ,pMsg->content,pMsg->length,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmd] post error\n");
                return;
        }
        NextState(RUNNING_STATE);
        emFileStatus = STATUS_SEND_UPLOAD;
#endif
}

void CCInstance::FileUploadCmdDeal(CMessage *const pMsg){

#if 1
        TFileList * tFile;
#endif

        //虽然是内部调用，但是为了断链处理，这边也做检测
#if MULTY_APP
        if(!m_bSignFlag){
#else
        if(!g_bSignFlag){
#endif
                OspLog(SYS_LOG_LEVEL,"[FileUploadCmdDeal]did not sign in\n");
                return;
        }

        if(!pMsg->content || pMsg->length <= 0){
                 OspLog(LOG_LVL_ERROR,"[FileUploadCmdDeal] pMsg is NULL\n");
                 printf("[FileUploadCmdDeal]msg is null\n");
                 return;
        }

        strcpy((LPSTR)file_name_path,(LPCSTR)pMsg->content);
        
        if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmdDeal]open file failed\n");
                printf("[FileUploadCmdDeal]open file failed\n");
                return;
        }
        //获取文件大小，根据标准c，二进制流SEEK_END没有严格得到支持，ftell返回值为long int，
        //32位系统大小限制在2G
        if(fseek(file,0L,SEEK_END) != 0){
                 OspLog(LOG_LVL_ERROR,"[FileUploadCmdDeal] file fseeek error\n");
                 return;
        }
        if(-1L == (m_wFileSize = ftell(file))){
                 OspLog(LOG_LVL_ERROR,"[FileUploadCmdDeal] file ftell error\n");
                 perror("file ftell error\n");
                 return;
        }

        m_wUploadFileSize = 0;
        rewind(file);

        if(OSP_OK != post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),FILE_SEND_UPLOAD
                       ,pMsg->content,pMsg->length,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmdDeal] post error\n");
                return;
        }

        emFileStatus = STATUS_SEND_UPLOAD;
        
#if 0
        //文件注册
#if THREAD_SAFE_MALLOC
        tFile = (TFileList*)malloc(sizeof(TFileList));
#else
        tFile = new TFileList();
#endif
        if(!tFile){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmdDeal]file list item malloc failed\n");
                //TODO：状态回收
                return;
        }
        strcpy((LPSTR)tFile->FileName,(LPSTR)pMsg->content);
        tFile->FileStatus = STATUS_SEND_UPLOAD;
        tFile->DealInstance = GetInsID();
        list_add(&tFile->tListHead,&tFileList);

#endif
#if 1
        if(!CheckFileIn((LPCSTR)file_name_path,&tFile)){
                OspLog(LOG_LVL_ERROR,"[FileUploadCmdDeal]file not in list\n");//客户端文件状态错误？
                //TODO:error deal
                return;
        }
        tFile->FileStatus = STATUS_UPLOADING;
#endif
        OspLog(SYS_LOG_LEVEL,"[FileUploadCmdDeal]send upload\n");
        //Daemon已经设置
        //NextState(RUNNING_STATE);
}

void CCInstance::SignInCmd(CMessage *const pMsg){


#if MULTY_APP
        if(!m_bConnectedFlag){
#else
        if(!g_bConnectedFlag){
#endif

#if 0
                //TODO:注册函数改为有返回值的，重新连接部分改为调用执行
                scanf("ip:%s port:%d\n",server_ip,server_port);
                g_dwdstNode = OspConnectTcpNode(inet_addr(SERVER_IP),SERVER_PORT,10,3);
                if(INVALID_NODE == g_dwdstNode){
                        OspLog(LOG_LVL_ERROR, "[SignInCmd]Connect to server faild.\n");
                        return;
                }else{
                        m_bConnectedFlag = true;
                }
#endif
                OspLog(SYS_LOG_LEVEL,"[SignInCmd]not connected\n");
                return;
        }

        if(!pMsg->content || pMsg->length <= 0){
                OspLog(LOG_LVL_ERROR,"[SignInCmd] pMsg content is NULL\n");
                return;
        }
#if MULTY_APP
        if(m_bSignFlag){
#else
        if(g_bSignFlag){
#endif
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

        int i;

#if MULTY_APP
        if(!m_bConnectedFlag){
#else
        if(!g_bConnectedFlag){
#endif
                OspLog(LOG_LVL_ERROR,"[SignInAck]disconnected\n");
                return;
        }

        if(pMsg->length <= 0 || !pMsg->content){
                OspLog(SYS_LOG_LEVEL,"[SignInAck]sign in failed\n");
                printf("[SignInAck]sign in failed\n");
                return;
                //TODO:向GUI发送重新登陆提示
        }
        if(strcmp((LPCSTR)pMsg->content,"failed") == 0){
                OspLog(SYS_LOG_LEVEL,"[SignInAck]sign in failed\n");
                printf("[SignInAck]sign in failed\n");
                return;
                //TODO:向GUI发送重新登陆提示
        }

#if MULTY_APP
        //通知登陆
        for(i = 0;i < CLIENT_APP_SUM;i++){
                if(post(MAKEIID(i+CLIENT_APP_ID,CInstance::DAEMON),MY_SIGNED),NULL,0){
                        OspLog(LOG_LVL_ERROR,"[SignInAck] post error\n");
                        //TODO:错误处理
                        return;
                }
        }
#else
        g_bSignFlag = true;
#endif
        OspLog(SYS_LOG_LEVEL,"[SignInAck]sign in successfully\n");
        printf("[SignInAck]sign in successfully\n");
}

void CCInstance::SignOutCmd(CMessage * const pMsg){

#if 0
        if(!m_bConnectedFlag){
                OspLog(LOG_LVL_ERROR,"[SignOutCmd]disconnected\n");
                return;
        }
#endif

#if MULTY_APP
        if(!m_bSignFlag){
#else
        if(!g_bSignFlag){
#endif
                OspLog(SYS_LOG_LEVEL,"[SignOutCmd]haven't sign in\n");
                return;
        }

#if 0
        //TODO:查询所有App instance状态，需要线程锁
        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SIGN_OUT_CMD
                               ,NULL,0)){
                        OspLog(LOG_LVL_ERROR,"[SignOutCmd] post error\n");
                }
                return;
        }

        if(emFileStatus == STATUS_UPLOADING){
                //TODO:返回让用户选择remove或者cancel
                //暂时默认remove
                if(OSP_OK != post(MAKEIID(GetAppID(),)
                                        ,SEND_REMOVE_CMD,NULL,0,g_dwdstNode)){
                        OspLog(LOG_LVL_ERROR,"[SignOutCmd] post error\n");
                        return;
                }
        }

#endif

        if(OSP_OK != post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON)
                                ,SIGN_OUT,NULL,0,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[SignOutCmd] post error\n");
                return;
        }
        OspLog(SYS_LOG_LEVEL,"get sign out cmd,send to server\n");
        OspPrintf(1,1,"get sign out cmd,send to server\n");

}

void CCInstance::SignOutAck(CMessage * const pMsg){

        int i;

#if MULTY_APP
        if(!m_bConnectedFlag){
#else
        if(!g_bConnectedFlag){
#endif
                OspLog(LOG_LVL_ERROR,"[SignOutAck]disconnected\n");
                return;
        }

#if MULTY_APP
          //通知登出
        for(i = 0;i < CLIENT_APP_SUM;i++){
                if(post(MAKEIID(i+CLIENT_APP_ID,CInstance::DAEMON),MY_DISSIGNED),NULL,0){
                        OspLog(LOG_LVL_ERROR,"[SignOutAck] post error\n");
                        //TODO:错误处理
                        return;
                }
        }
#else
        g_bSignFlag = false;
#endif
        //TODO:回收状态，暂停所有任务
        
        OspLog(SYS_LOG_LEVEL,"[SignOutAck]sign out\n");
        printf("[SignOutAck]get sign out ack\n");
}

#if 0
void CCInstance::FileReceiveUploadAck(CMessage * const pMsg){

        size_t buffer_size;
        TFileList *tFile;
#if MULTY_APP
        if(!m_bSignFlag){
#else
        if(!g_bSignFlag){
#endif
                OspLog(SYS_LOG_LEVEL,"[FileReceiveUploadAck]sign out\n");
                return;
        }

        if(!CheckFileIn((LPCSTR)file_name_path,&tFile)){
                OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]file not in list\n");//客户端文件状态错误？
                //TODO:error deal
                return;
        }

        m_dwDisInsID = pMsg->srcid;

        buffer_size = fread(buffer,1,sizeof(s8)*BUFFER_SIZE,file);
        if(ferror(file)){
                if(fclose(file) == 0){
                        OspLog(SYS_LOG_LEVEL,"[FileReceiveUploadAck]file closed\n");
                        file = NULL;
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
        tFile->FileStatus = STATUS_UPLOADING;
        emFileStatus = STATUS_UPLOADING;
        printf("upload file rate:%f\n",(float)m_wUploadFileSize/(float)m_wFileSize);
}
#endif

void CCInstance::FileUploadAck(CMessage* const pMsg){

        size_t buffer_size;
#if MULTY_APP
        if(!m_bSignFlag){
#else
        if(!g_bSignFlag){
#endif
                OspLog(LOG_LVL_ERROR,"[FileUploadAck]sign out\n");
                return;
        }

        if(!pMsg->content || pMsg->length <= 0){
                 OspLog(LOG_LVL_ERROR,"[FileUploadAck] pMsg content is NULL\n");
                 return;
        }

        m_dwDisInsID = pMsg->srcid;
        emFileStatus = *((EM_FILE_STATUS*)pMsg->content);
        if(emFileStatus == STATUS_UPLOADING){//继续发送
                buffer_size = fread(buffer,1,sizeof(s8)*BUFFER_SIZE,file);
                if(ferror(file)){
                        if(fclose(file) == 0){
                                OspLog(SYS_LOG_LEVEL,"[FileUploadAck]file closed\n");
                        }else{
                                OspLog(LOG_LVL_ERROR,"[FileUploadAck]file close failed\n");
                        }
                        file = NULL;
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
                OspLog(LOG_LVL_ERROR,"[FileUploadAck]get incorrect file status:%d\n",emFileStatus);
                printf("[FileUploadAck]get incorrect file status\n");
        }
}

void CCInstance::FileFinishAck(CMessage* const pMsg){
        
        TFileList *tFile;

        if(!g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[FileFinishAck]did not sign in\n");
                return;
        }

        if(fclose(file) == 0){
                OspLog(SYS_LOG_LEVEL,"[FileFinishAck]file closed\n");

        }else{
                OspLog(LOG_LVL_ERROR,"[FileFinishAck]file close failed\n");
        }
        file = NULL;

        if(!CheckFileIn((LPCSTR)file_name_path,&tFile)){
                OspLog(LOG_LVL_ERROR,"[FileFinishAck]file not in list\n");//客户端文件状态错误？
                //TODO:error deal
                return;
        }
        tFile->FileStatus = STATUS_FINISHED;

#if 0
        list_del(&tFile->tListHead);
#if THREAD_SAFE_MALLOC
        free(tFile);
#else
        delete tFile;
#endif
#endif

        OspLog(SYS_LOG_LEVEL,"[FileFinishAck]file upload finish\n");
        emFileStatus = STATUS_FINISHED;
        NextState(IDLE_STATE);
#if 0
        m_wFileSize = 0;
        m_wUploadFileSize = 0;
#endif
}

void CCInstance::MsgProcessInit(){

        //Daemon Instance
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_CMD),&CCInstance::SignInCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_OUT_CMD),&CCInstance::SignOutCmd,&m_tCmdDaemonChain);

        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_OUT_ACK),&CCInstance::SignOutAck,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SIGN_IN_ACK),&CCInstance::SignInAck,&m_tCmdDaemonChain);

        RegMsgProFun(MAKEESTATE(IDLE_STATE,OSP_DISCONNECT),&CCInstance::notifyDisconnect,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_UPLOAD_CMD),&CCInstance::FileUploadCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SEND_CANCEL_CMD),&CCInstance::CancelCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_GO_ON_CMD),&CCInstance::FileGoOnCmd,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SEND_REMOVE_CMD),&CCInstance::RemoveCmd,&m_tCmdDaemonChain);
#if MULTY_APP
        RegMsgProFun(MAKEESTATE(IDLE_STATE,MY_CONNECT),&CCInstance::notifyConnect,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,MY_SIGNED),&CCInstance::notifySigned,&m_tCmdDaemonChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,MY_DISSIGNED),&CCInstance::notifyDissigned,&m_tCmdDaemonChain);
#endif


        //common Instance
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_UPLOAD_ACK),&CCInstance::FileUploadAck,&m_tCmdChain);
#if 0
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_RECEIVE_UPLOAD_ACK),&CCInstance::FileReceiveUploadAck
                        ,&m_tCmdChain);
#endif
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_FINISH_ACK),&CCInstance::FileFinishAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_CANCEL_ACK),&CCInstance::FileCancelAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_REMOVE_ACK),&CCInstance::FileRemoveAck,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_GO_ON_ACK),&CCInstance::FileGoOnAck,&m_tCmdChain);
        
             //Deal Instance
             //Deamon 将这些Instance状态设置为RUNNING，防止因为立刻调用查询空闲函数导致这些instance
             //被其他任务(业务)查询之后调用，对于本地延迟没有效果，不考虑本地延迟

        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_UPLOAD_CMD_DEAL),&CCInstance::FileUploadCmdDeal,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SEND_REMOVE_CMD_DEAL),&CCInstance::RemoveCmdDeal,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SEND_CANCEL_CMD_DEAL),&CCInstance::CancelCmdDeal,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,SEND_STABLE_REMOVE_CMD_DEAL),&CCInstance::StableRemoveCmdDeal,
                        &m_tCmdChain);
        RegMsgProFun(MAKEESTATE(RUNNING_STATE,FILE_GO_ON_CMD_DEAL),&CCInstance::FileGoOnCmdDeal,&m_tCmdChain);

#if MULTY_APP
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_UPLOAD_CMD),&CCInstance::FileUploadCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,GET_MY_CONNECT),&CCInstance::GetMyConnect,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,GET_SIGNED),&CCInstance::GetSigned,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,GET_DISSIGNED),&CCInstance::GetDissigned,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,GET_DISCONNECT),&CCInstance::GetDisconnect,&m_tCmdChain);
#endif


        //直接返回不处理，调试信息完整，避免只返回can not find the EState
        RegMsgProFun(MAKEESTATE(IDLE_STATE,SEND_CANCEL_CMD),&CCInstance::CancelCmd,&m_tCmdChain);
        RegMsgProFun(MAKEESTATE(IDLE_STATE,FILE_GO_ON_CMD),&CCInstance::FileGoOnCmd,&m_tCmdChain);

}

void CCInstance::NodeChainEnd(){

        tCmdNode *tmpNode;

        while(m_tCmdChain){
                tmpNode = m_tCmdChain->next;
#if THREAD_SAFE_MALLOC
                free(m_tCmdChain);
#else
                delete m_tCmdChain;
#endif
                m_tCmdChain = tmpNode;
        }

        while(m_tCmdDaemonChain){
                tmpNode = m_tCmdChain->next;
#if THREAD_SAFE_MALLOC
                free(m_tCmdDaemonChain);
#else
                delete m_tCmdDaemonChain;
#endif
                m_tCmdDaemonChain = tmpNode;
        }
}

bool CCInstance::RegMsgProFun(u32 EventState,MsgProcess c_MsgProcess,tCmdNode** tppNodeChain){

        tCmdNode *Node,*NewNode,*LNode;

        Node = *tppNodeChain;

#if THREAD_SAFE_MALLOC
        if(!(NewNode = (tCmdNode*)malloc(sizeof(tCmdNode)))){
#else
        if(!(NewNode = new tCmdNode())){
#endif
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

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        MsgProcess c_MsgProcess;

#if 0
        if(!m_bConnectedFlag){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry]disconnected\n");
                return;
        }
        if(!m_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[InstanceEntry]sign out\n");
                return;
        }
#endif

        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
                return;
        }

        if(FindProcess(MAKEESTATE(curState,curEvent),&c_MsgProcess,m_tCmdChain)){
                (this->*c_MsgProcess)(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] can not find the EState,event:%d,state:%d\n"
                                ,curEvent,curState);
                printf("[InstanceEntry] can not find the EState\n");
        }

}

void CCInstance::DaemonInstanceEntry(CMessage *const pMsg,CApp *pCApp){

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        MsgProcess c_MsgProcess;

        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[DaemonInstanceEntry] pMsg is NULL\n");
                return;
        }

        if(FindProcess(MAKEESTATE(curState,curEvent),&c_MsgProcess,m_tCmdDaemonChain)){
                (this->*c_MsgProcess)(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[DaemonInstanceEntry] can not find the EState,event:%d,state:%d\n"
                                ,curEvent,curState);
                printf("[DaemonInstanceEntry] can not find the EState\n");
        }
}


void CCInstance::RemoveCmd(CMessage* const pMsg){

    TFileList *tFile;
	CCInstance *ccIns;

    if(!g_bSignFlag){
            OspLog(SYS_LOG_LEVEL,"[RemoveCmd]did not sign in\n");
            return;
    }

    if(!pMsg->content || pMsg->length <= 0){
             OspLog(LOG_LVL_ERROR,"[RemoveCmd]Msg is NULL\n");
             printf("[RemoveCmd]msg is null\n");
             return;
    }

    if(!CheckFileIn((LPCSTR)pMsg->content,&tFile)){
            OspLog(LOG_LVL_ERROR,"[RemoveCmd]file not in list\n");//客户端文件状态错误
            return;
    }

#if 0
        //延迟处理，重新放入队列
    if(tFile->FileStatus >= STATUS_UPLOAD_CMD
                    && tFile->FileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SEND_REMOVE_CMD
                               ,pMsg->content,pMsg->length)){
                        OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                }
                return;

    }
#endif
    if(tFile->FileStatus == STATUS_UPLOADING){
            if(OSP_OK != post(MAKEIID(CLIENT_APP_ID,tFile->DealInstance),SEND_REMOVE_CMD_DEAL
                           ,pMsg->content,strlen((LPCSTR)pMsg->content)+1)){
                    OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                    //TODO:状态回收
                    return;
            }
            tFile->FileStatus = STATUS_REMOVE_CMD;
            return;
    }

    if(tFile->FileStatus == STATUS_REMOVED){
            OspLog(SYS_LOG_LEVEL, "[RemoveCmd]file already removed\n");
            return;
    }
     //查询空闲Instance
    if(!(ccIns = GetPendingIns())){
            OspLog(SYS_LOG_LEVEL, "[RemoveCmd]no pending instance,please wait...\n");
            return;
    }
    if(OSP_OK != post(MAKEIID(CLIENT_APP_ID,ccIns->GetInsID()),SEND_STABLE_REMOVE_CMD_DEAL
                   ,pMsg->content,strlen((LPCSTR)pMsg->content)+1)){
            OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
            //TODO:状态回收
            return;
    }
    ccIns->m_curState = RUNNING_STATE;
    tFile->FileStatus = STATUS_REMOVE_CMD;

}

void CCInstance::RemoveCmdDeal(CMessage* const pMsg){ 

    if(!g_bSignFlag){
            OspLog(SYS_LOG_LEVEL,"[RemoveCmdDeal]did not sign in\n");
            return;
    }

    if(OSP_OK != post(m_dwDisInsID,SEND_REMOVE
                   ,pMsg->content,pMsg->length,g_dwdstNode)){
            OspLog(LOG_LVL_ERROR,"[RemoveCmdDeal] post error\n");
            return;
    }
    emFileStatus = STATUS_SEND_REMOVE;
//    tFile->FileStatus = STATUS_SEND_REMOVE;
}

void CCInstance::StableRemoveCmdDeal(CMessage* const pMsg){ 

    if(!g_bSignFlag){
            OspLog(SYS_LOG_LEVEL,"[StableRemoveCmdDeal]did not sign in\n");
            return;
    }

    if(OSP_OK != post(MAKEIID(SERVER_APP_ID,DAEMON),FILE_STABLE_REMOVE
                   ,pMsg->content,pMsg->length,g_dwdstNode)){
            OspLog(LOG_LVL_ERROR,"[StableRemoveCmdDeal] post error\n");
            return;
    }


    strcpy((LPSTR)file_name_path,(LPCSTR)pMsg->content);
    emFileStatus = STATUS_SEND_REMOVE;
    //tFile->FileStatus = STATUS_SEND_REMOVE;
}

void CCInstance::FileRemoveAck(CMessage* const pMsg){

        TFileList *tFile;
        bool *bStableFlag;

        bStableFlag = (bool*)pMsg->content;

        if(!g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[FileRemoveAck]did not sign in\n");
                return;
        }

        if(!*bStableFlag){
                if(fclose(file) == 0){
                        OspLog(SYS_LOG_LEVEL,"[FileRemoveAck]file closed\n");
                }else{
                        OspLog(LOG_LVL_ERROR,"[FileRemoveAck]file close failed\n");
                }
                file = NULL;
        }

        if(!CheckFileIn((LPCSTR)file_name_path,&tFile)){
                OspLog(LOG_LVL_ERROR,"[FileRemoveAck]file not in list\n");//客户端文件状态错误？
                //TODO:error deal
                return;
        }
        tFile->FileStatus = STATUS_REMOVED;
#if 0
        list_del(&tFile->tListHead);
#if THREAD_SAFE_MALLOC
        free(tFile);
#else
        delete tFile;
#endif
#endif

        emFileStatus = STATUS_REMOVED;
#if 0
        m_wUploadFileSize = 0;
        m_wFileSize = 0;
#endif
        NextState(IDLE_STATE);
        //TODO:key print
        printf("file remove\n");
}

#if MULTY_APP
void CCInstance::FileGoOnCmd(CMessage* const pMsg){

        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),FILE_GO_ON_CMD
                               ,NULL,0)){
                        OspLog(LOG_LVL_ERROR,"[RemoveCmd] post error\n");
                }
                return;
        }
        
        //从GUI的使用来说，这个可以保证
        if(emFileStatus == STATUS_INIT){
                OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]status error,upload first\n");
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
                               ,NULL,0)){
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
#endif

//single app

void CCInstance::CancelCmd(CMessage* const pMsg){

        TFileList *tFile;

        if(!g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[CancelCmd]did not sign in\n");
                return;
        }

        if(!pMsg->content || pMsg->length <= 0){
                 OspLog(LOG_LVL_ERROR,"[CancelCmd]Msg is NULL\n");
                 printf("[CancelCmd]msg is null\n");
                 return;
        }

        if(!CheckFileIn((LPCSTR)pMsg->content,&tFile)){
                OspLog(LOG_LVL_ERROR,"[CancelCmd]file not in list\n");//客户端文件状态错误？
                return;
        }

#if 0
        //延迟处理，重新放入队列
        if(tFile->FileStatus >= STATUS_UPLOAD_CMD
                        && tFile->FileStatus <= STATUS_RECEIVE_REMOVE){
                    if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SEND_CANCEL_CMD
                                   ,pMsg->content,pMsg->length)){
                            OspLog(LOG_LVL_ERROR,"[CancelCmd] post error\n");
                    }
                    return;

        }
#endif

        //检查是否是uploading状态
        if(tFile->FileStatus > STATUS_UPLOADING){
                OspLog(LOG_LVL_ERROR,"[CancelCmd]file already stable\n");
                return;
        }

        if(OSP_OK != post(MAKEIID(CLIENT_APP_ID,tFile->DealInstance),SEND_CANCEL_CMD_DEAL
                       ,pMsg->content,pMsg->length)){
                OspLog(LOG_LVL_ERROR,"[CancelCmd] post error\n");
                return;
        }
        tFile->FileStatus = STATUS_CANCEL_CMD;
}

void CCInstance::CancelCmdDeal(CMessage* const pMsg){

        if(!g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[CancelCmdDeal]did not sign in\n");
                return;
        }

#if 0
        //未处理完状态，重新放入消息队列等待状态稳定
        if(emFileStatus >= STATUS_SEND_UPLOAD&&
                        emFileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),SEND_CANCEL_CMD_DEAL
                               ,NULL,0)){
                        OspLog(LOG_LVL_ERROR,"[CancelCmdDeal] post error\n");
                }
                return;
        }

        if(emFileStatus >= STATUS_CANCELLED){
                OspLog(SYS_LOG_LEVEL,"[CancelCmdDeal]Already stable state\n");
                return;
                
        }
#endif
        if(OSP_OK != post(m_dwDisInsID,SEND_CANCEL
                       ,pMsg->content,pMsg->length,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[CancelCmdDeal] post error\n");
                return;
        }

        OspLog(SYS_LOG_LEVEL,"[CancelCmdDeal]send cancel\n");
        emFileStatus = STATUS_SEND_CANCEL;
}

void CCInstance::FileCancelAck(CMessage* const pMsg){

        u16 wAppId;
        TFileList *tFile;

        if(fclose(file) == 0){
                OspLog(SYS_LOG_LEVEL,"[FileCancelAck]file closed\n");
        }else{
                OspLog(LOG_LVL_ERROR,"[FileCancelAck]file close failed\n");
        }
        file = NULL;

        //修改文件表中状态
        if(!CheckFileIn((LPCSTR)file_name_path,&tFile)){
                OspLog(LOG_LVL_ERROR,"[FileCancelAck]file not in list\n");//客户端文件状态错误？
                //TODO:error deal
                return;
        }
        tFile->FileStatus = STATUS_CANCELLED;
        tFile->UploadFileSize = m_wUploadFileSize;
        tFile->FileSize = m_wFileSize;
        emFileStatus = STATUS_CANCELLED;
	    //释放instance
        NextState(IDLE_STATE);
}

void CCInstance::FileGoOnCmd(CMessage* const pMsg){

    TFileList *tFile;
    TDemoInfo tDemoInfo;
	CCInstance *ccIns;

    if(!g_bSignFlag){
            OspLog(SYS_LOG_LEVEL,"[FileGoOnCmd]did not sign in\n");
            return;
    }

    if(!pMsg->content || pMsg->length <= 0){
             OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]Msg is NULL\n");
             printf("[FileGoOnCmd]msg is null\n");
             return;
    }

    if(!CheckFileIn((LPCSTR)pMsg->content,&tFile)){
            OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]file not in list\n");//客户端文件状态错误
            return;
    }

#if 0
    if(tFile->FileStatus >= STATUS_UPLOAD_CMD
                    && tFile->FileStatus <= STATUS_RECEIVE_REMOVE){
                if(OSP_OK != post(MAKEIID(GetAppID(),GetInsID()),FILE_GO_ON_CMD
                               ,pMsg->content,pMsg->length)){
                        OspLog(LOG_LVL_ERROR,"[FileGoOnCmd] post error\n");
                }
                return;

    }
#endif

	if(tFile->FileStatus != STATUS_CANCELLED){
                OspLog(LOG_LVL_ERROR,"[FileGoOnCmd]file upload not cancelled\n");
                return;
	}

        //查询空闲Instance
    if(!(ccIns = GetPendingIns())){
            OspLog(SYS_LOG_LEVEL, "[FileGoOnCmd]no pending instance,please wait...\n");
            return;
    }
    strcpy((LPSTR)tDemoInfo.FileName,(LPCSTR)tFile->FileName);
    tDemoInfo.UploadFileSize = tFile->UploadFileSize;
    tDemoInfo.FileSize = tFile->FileSize;
    if(OSP_OK != post(MAKEIID(CLIENT_APP_ID,ccIns->GetInsID()),FILE_GO_ON_CMD_DEAL
                   ,&tDemoInfo,sizeof(tDemoInfo))){
            OspLog(LOG_LVL_ERROR,"[FileGoOnCmd] post error\n");
            //TODO:状态回收
            return;
    }
    ccIns->m_curState = RUNNING_STATE;
    tFile->FileStatus = STATUS_GO_ON_CMD;
    tFile->DealInstance = ccIns->GetInsID();
}

void CCInstance::FileGoOnCmdDeal(CMessage* const pMsg){

        TDemoInfo *tDemoInfo;

        tDemoInfo = (TDemoInfo*)pMsg->content;

        if(!g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[FileGoOnCmdDeal]did not sign in\n");
                return;
        }

        m_wUploadFileSize = tDemoInfo->UploadFileSize;
        m_wFileSize = tDemoInfo->FileSize;

        if(!(file = fopen((LPCSTR)tDemoInfo->FileName,"rb"))){
                OspLog(LOG_LVL_ERROR,"[FileGoOnAck]open file failed\n");
                printf("[FileGoOnAck]open file failed\n");
                return;
        }

        if(fseek(file,m_wUploadFileSize,SEEK_SET) != 0){
                 OspLog(LOG_LVL_ERROR,"[FileGoOnAck] file fseeek error\n");
                 return;
        }

        if(OSP_OK != post(MAKEIID(SERVER_APP_ID,DAEMON),FILE_GO_ON
                       ,tDemoInfo->FileName,strlen(tDemoInfo->FileName)+1,g_dwdstNode)){
                OspLog(LOG_LVL_ERROR,"[FileGoOnCmdDeal] post error\n");
                return;
        }

        strcpy((LPSTR)file_name_path,(LPCSTR)tDemoInfo->FileName);
//        emFileStatus = STATUS_SEND_CANCEL;
}

#if 1
void CCInstance::FileGoOnAck(CMessage* const pMsg){

        size_t buffer_size;

        if(!g_bSignFlag){
                OspLog(SYS_LOG_LEVEL,"[FileGoOnAck]did not sign in\n");
                return;
        }
 
        if(!(file = fopen((LPCSTR)file_name_path,"rb"))){
                OspLog(LOG_LVL_ERROR,"[FileGoOnAck]open file failed\n");
                printf("[FileGoOnAck]open file failed\n");
                return;
        }

        if(fseek(file,m_wUploadFileSize,SEEK_SET) != 0){
                 OspLog(LOG_LVL_ERROR,"[FileGoOnAck] file fseeek error\n");
                 return;
        }

        m_dwDisInsID = pMsg->srcid;
        buffer_size = fread(buffer,1,sizeof(s8)*BUFFER_SIZE,file);
        if(ferror(file)){
                if(fclose(file) == 0){
                        OspLog(SYS_LOG_LEVEL,"[FileGoOnAck]file closed\n");
                }else{
                        OspLog(LOG_LVL_ERROR,"[FileGoOnAck]file close failed\n");
                }
                file = NULL;

                //TODO:通知server关闭文件
                OspLog(LOG_LVL_ERROR,"[FileGoOnAck] read-file error\n");
                return;
        }

        if(feof(file)){//文件已读取完毕，终止
             if(OSP_OK != post(pMsg->srcid,FILE_FINISH
                           ,buffer,buffer_size,g_dwdstNode)){
                  OspLog(LOG_LVL_ERROR,"[FileGoOnAck]FILE_FINISH post error\n");
                  return;
             }
        
        }else{
             if(OSP_OK != post(pMsg->srcid,FILE_UPLOAD
                           ,buffer,buffer_size,g_dwdstNode)){
                  OspLog(LOG_LVL_ERROR,"[FileGoOnAck]FILE_UPLOAD post error\n");
                  return;
             }
        }
        m_wUploadFileSize += buffer_size;
        printf("upload file rate:%f\n",(float)m_wUploadFileSize/(float)m_wFileSize);
}
#endif

void CCInstance::notifyDisconnect(CMessage* const pMsg){

        //TODO:断开之后状态需要回收
        u16 i;
        CCInstance *pIns;
#if MULTY_APP
        m_bConnectedFlag = false;
        m_bSignFlag = false;

        if(OSP_OK != post(MAKEIID(GetAppID(),CLIENT_INSTANCE_ID),GET_DISCONNECT
                       ,NULL,0)){
                OspLog(LOG_LVL_ERROR,"[notifyDisConnect] post error\n");
        }
#else
        struct list_head *tFileHead,*templist;
        TFileList *tnFile;

        if(!g_bConnectedFlag){
                OspLog(LOG_LVL_ERROR,"[notifyDisConnect] not connected\n");
                return;
        }
        g_bConnectedFlag = false;
        g_bSignFlag = false;

        //TODO:断点续传
        list_for_each_safe(tFileHead,templist,&tFileList){
                tnFile = list_entry(tFileHead,TFileList,tListHead);
                list_del(&(tnFile->tListHead));
#if THREAD_SAFE_MALLOC
                free(tnFile);
#else
                delete tnFile;
#endif
        }
        INIT_LIST_HEAD(&tFileList);

        for(i = 1;i < MAX_INS_NUM;i++){
               pIns = (CCInstance*)((CApp*)&g_cCApp)->GetInstance(i);
               if(!pIns){
                   OspLog(LOG_LVL_ERROR,"[notifyDisConnect]get error ins\n");
                   continue;
               }
               pIns->m_curState = CInstance::PENDING;
               pIns->emFileStatus = STATUS_INIT;
               if(pIns->file){
                       if(fclose(pIns->file) == 0){
                               OspLog(SYS_LOG_LEVEL,"[notifyDisConnect]file closed\n");
                               pIns->file = NULL;
                       }else{
                               OspLog(LOG_LVL_ERROR,"[notifyDisConnect]file close failed\n");
                               return;
                       }
               }
        }
#if 0
        if(OSP_OK != OspNodeDelDiscCB(g_dwdstNode,CLIENT_APP_ID,CInstance::DAEMON)){
               OspLog(LOG_LVL_ERROR,"[notifyDisConnect]del discb failed\n");
        }
#endif
        OspLog(SYS_LOG_LEVEL, "[notifyDisConnect]disconnect\n");
#endif
}

#if MULTY_APP

void CCInstance::GetDisconnect(CMessage* const pMsg){

        //TODO:断开之后状态需要回收
        m_bConnectedFlag = false;
        m_bSignFlag = false;

        //TODO：断点续传
        if(file){
                if(fclose(file) == 0){
                        OspLog(SYS_LOG_LEVEL,"[FileRemovelAck]file closed\n");
                        file = NULL;
                }else{
                        OspLog(LOG_LVL_ERROR,"[FileRemoveAck]file close failed\n");
                        return;
                }
        }
        //需要配合文件的关闭回收
        emFileStatus = STATUS_INIT;
        NextState(IDLE_STATE);
        m_dwDisInsID = 0;
        m_wFileSize = 0;
        m_wUploadFileSize = 0;
        //m_wServerPort = SERVER_PORT;
        OspLog(SYS_LOG_LEVEL,"[GetDisconnect]disconnected\n");
}

void CCInstance::notifyConnect(CMessage* const pMsg){

        m_bConnectedFlag = true;
        if(OSP_OK != post(MAKEIID(GetAppID(),CLIENT_INSTANCE_ID),GET_MY_CONNECT
                       ,NULL,0)){
                OspLog(LOG_LVL_ERROR,"[notifyConnect] post error\n");
        }
}

void CCInstance::GetMyConnect(CMessage* const pMsg){

        m_bConnectedFlag = true;
        OspLog(SYS_LOG_LEVEL,"[GetMyConnect]server connected\n");
}


void CCInstance::notifySigned(CMessage* const pMsg){

        m_bSignFlag = true;
        if(OSP_OK != post(MAKEIID(GetAppID(),CLIENT_INSTANCE_ID),GET_SIGNED
                       ,NULL,0)){
                OspLog(LOG_LVL_ERROR,"[notifySigned] post error\n");
        }

        OspLog(SYS_LOG_LEVEL,"[notifySigned]sign in\n");
}

void CCInstance::GetSigned(CMessage* const pMsg){

        m_bSignFlag = true;
}

void CCInstance::GetDissigned(CMessage* const pMsg){

        m_bSignFlag = false;
}

void CCInstance::notifyDissigned(CMessage* const pMsg){

        m_bSignFlag = false;
        if(OSP_OK != post(MAKEIID(GetAppID(),CLIENT_INSTANCE_ID),GET_DISSIGNED
                       ,NULL,0)){
                OspLog(LOG_LVL_ERROR,"[notifyDissigned] post error\n");
        }

        OspLog(SYS_LOG_LEVEL,"[notifyDissigned]sign out\n");
}
#endif

static bool CheckFileIn(LPCSTR filename,TFileList **tFile){

        struct list_head *tFileHead;
        TFileList *tnFile = NULL;
        bool inFileList = false;

        list_for_each(tFileHead,&tFileList){
                tnFile = list_entry(tFileHead,TFileList,tListHead);
                if(0 == strcmp((LPCSTR)tnFile->FileName,filename)){
                        inFileList = true;
                        break;
                }
        }
        if(tFile){
                *tFile = tnFile;
        }
        return inFileList;
}

static CCInstance* GetPendingIns(){

       u16 instCount;
       CCInstance* pIns;

       g_cCApp.wLastIdleInstID %= MAX_INS_NUM;
	   instCount = g_cCApp.wLastIdleInstID;
	   do{
               instCount++;
               pIns = (CCInstance*)((CApp*)&g_cCApp)->GetInstance(instCount);
               if( pIns->CurState() == CInstance::PENDING ) {
                    break;
               }
               instCount %= MAX_INS_NUM;
	   } while( instCount != g_cCApp.wLastIdleInstID );

	   if( instCount == g_cCApp.wLastIdleInstID ){
               //TODO:通知客户端，没有找到空闲实例
               return NULL;
       }
	   g_cCApp.wLastIdleInstID = instCount;
       return pIns;

}

