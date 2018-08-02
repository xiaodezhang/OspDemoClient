#include"osp.h"
#include"client.h"
#include"list.h"
#include"demogui.h"


typedef  void (*msgProcess)(CMessage*const);

typedef struct tagInsNode{
        struct list_head        tListHead;
        u32                     EventState;
        msgProcess  c_MsgProcess;
        struct      tagCmdNode *next;
}TInsNode;

static GuiApp g_GuiApp;
static struct list_head  TInsNodeHead;


static TInsNode* findProcess(const u32 eventState,const struct list_head* tInsNodeHead);
static void regProcess(const u32 eventState,const msgProcess c_MsgProcess
                                ,struct list_head* tInsNodeHead);
static void delInsNode(struct list_head * tInsNodeHead);

void msgProcessInit();

int main(){

#ifdef _MSC_VER
        int ret = OspInit(TRUE,2500,"WindowsOspClient");
#else
        int ret = OspInit(TRUE,2500,"LinuxOspClient");
#endif
        bool bCreateTcpNodeFlag = false;
        u16 i;

        if(!ret){
                OspPrintf(1,0,"osp init fail\n");
                OspQuit();
                return -1;
        }

        INIT_LIST_HEAD(&TInsNodeHead);
        msgProcessInit();

        if(OSP_OK != g_GuiApp.CreateApp("GuiApp",GUI_APP_ID,GUI_APP_PRI,MAX_MSG_WAITING)){
                OspLog(LOG_LVL_ERROR,"[main]app create error\n");
                delInsNode(&TInsNodeHead);
                OspQuit();
                return -2;
        }

        //尝试多次建立node，支持本地多个客户端的副本
        for(i = 0;i < CREATE_TCP_NODE_TIMES;i++){
                if(INVALID_SOCKET != (ret = OspCreateTcpNode(0,DEMO_GUI_LISTEN_PORT+i*3))){
                        bCreateTcpNodeFlag = true;
                        break;
                }
        }
        if(!bCreateTcpNodeFlag){
                OspQuit();
                OspLog(SYS_LOG_LEVEL,"[main]create positive node failed,quit\n");
                printf("[main]node created failed\n");
                return -3;
        }


        if(0 != clientInit(DEMO_GUI_LISTEN_PORT+i*3)){
                OspLog(LOG_LVL_ERROR,"[main]client init error\n");
                delInsNode(&TInsNodeHead);
                OspQuit();
                return -1;
        }

        while(1)
                OspDelay(100);

        delInsNode(&TInsNodeHead);
        OspQuit();

        return 0;
}


void GuiInstance::InstanceEntry(CMessage *const pMsg){

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        msgProcess c_MsgProcess;
        TInsNode *fInsNode;

        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry]msg is NULL\n");
                return;
        }

        if((fInsNode = findProcess(MAKEESTATE(curState,curEvent),&TInsNodeHead))){
                fInsNode->c_MsgProcess(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[InstanceEntry]state or event error\n");
        }
}

void GuiInstance::DaemonInstanceEntry(CMessage *const pMsg,CApp* pApp){

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        msgProcess c_MsgProcess;
        TInsNode *fInsNode;

        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry]msg is NULL\n");
                return;
        }

        if((fInsNode = findProcess(MAKEESTATE(curState,curEvent),&TInsNodeHead))){
                fInsNode->c_MsgProcess(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[InstanceEntry]state or event error,event:%d, \
                                state:%d\n",curEvent,curState);
        }
}

void GetSignIn(CMessage* const pMsg){
        printf("[GetSignIn]ack:%d\n",*(s16*)pMsg->content);
}

void GetSignOut(CMessage* const pMsg){
        printf("[GetSignOut]ack:%d\n",*(s16*)pMsg->content);
}

void GetFileSize(CMessage* const pMsg){
        printf("[GetFileSize]ack:%ld\n",*(u64*)pMsg->content);
}

void GetUploadFileSize(CMessage* const pMsg){
//        printf("[GetUploadFileSize]ack:%ld\n",*(u64*)pMsg->content);
}

void msgProcessInit(){

        regProcess(MAKEESTATE(IDLE_STATE,GUI_SIGN_IN_ACK),GetSignIn,&TInsNodeHead);
        regProcess(MAKEESTATE(IDLE_STATE,GUI_SIGN_OUT_ACK),GetSignOut,&TInsNodeHead);
        regProcess(MAKEESTATE(IDLE_STATE,GUI_FILE_SIZE_ACK),GetFileSize,&TInsNodeHead);
        regProcess(MAKEESTATE(IDLE_STATE,GUI_UPLOAD_FILE_SIZE_ACK),GetUploadFileSize,&TInsNodeHead);
}

static TInsNode* findProcess(const u32 eventState,const struct list_head* tInsNodeHead){

        TInsNode *tnInsNode;
        struct list_head *insHead;

        list_for_each(insHead,tInsNodeHead){
                tnInsNode = list_entry(insHead,TInsNode,tListHead);
                if(tnInsNode->EventState == eventState){
                        return tnInsNode;
                }
                
        }
        return NULL;
}

static void regProcess(const u32 eventState,const msgProcess c_MsgProcess
                                ,struct list_head* tInsNodeHead){

        TInsNode* tInsNode = NULL;

        if(!(tInsNode = new TInsNode())){
                OspLog(LOG_LVL_ERROR,"[regProcess]node malloc error\n");
                return;
        }

        if(findProcess(eventState,tInsNodeHead)){
                OspLog(LOG_LVL_ERROR,"[regProcess]node already registered\n");
                return;
        }

        tInsNode->c_MsgProcess = c_MsgProcess;
        tInsNode->EventState = eventState;
        list_add(&tInsNode->tListHead,tInsNodeHead);
}

static void delInsNode(struct list_head* tInsNodeHead){

        struct list_head *insHead,*templist;
        TInsNode *tnInsNode;

        list_for_each_safe(insHead,templist,tInsNodeHead){
                tnInsNode = list_entry(insHead,TInsNode,tListHead);
                list_del(&tnInsNode->tListHead);
                delete tnInsNode;
        }

        INIT_LIST_HEAD(tInsNodeHead);
}

