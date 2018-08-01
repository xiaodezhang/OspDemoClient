#include"osp.h"
#include"client.h"
#include"list.h"
#include"demogui.h"

#define DEMO_GUI_LISTEN_PORT                    5000
#define GUI_APP_ID                              (u16)4
#define GUI_APP_PRI                             (u8)80

typedef msgPorcess void (*process_fun)(CMessage*);

typedef struct tagInsNode{
        struct list_head        tListHead;
        u32                     EventState;
        msgProcess  c_MsgProcess;
        struct      tagCmdNode *next;
}TInsNode;

static GuiApp g_GuiApp;
static struct list_head  TInsNodeHead;


u32 DemoGuiInit(){

#if 0
#ifdef _MSC_VER
        int ret = OspInit(TRUE,2500,"WindowsOspClient");
#else
        int ret = OspInit(TRUE,2500,"LinuxOspClient");
#endif
        bool bCreateTcpNodeFlag = false;

        if(!ret){
                OspPrintf(1,0,"osp init fail\n");
                return -1;
        }
#endif

        if(OSP_OK != g_GuiApp.CreateApp("GuiApp",GUI_APP_ID,GUI_APP_PRI,MAX_MSG_WAITING)){
                OspLog(LOG_LVL_ERROR,"[DemoGuiInit]app create error\n");
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

        INIT_LIST_HEAD(&TInsNodeHead);

        return OSP_AGENT_CLIENT_PORT+i*3;
}

static TInsNode* findProcess(const TInsNode* tInsNode,const struct list_head* tInsNodeHead){

        TInsNode *tnInsNode = NULL;
        struct list_head *insHead;

        list_for_each(tnInsNode,tInsNodeHead){
                tnInsNode = list_entry(tnInsNode,TInsNode,tListHead);
                if(tnInsNode->EventState == tInsNode->EventState){
                        break;
                }
                
        }
        return tnInsNode;
}

static void regProcess(const u32 eventState,const msgProcess* c_MsgProcess
                                ,struct list_head** tInsNodeHead){

        TInsNode* tInsNode = NULL;

        if(!(tInsNode = new TInsNode())){
                OspLog(LOG_LVL_ERROR,"[regProcess]node malloc error\n");
                return;
        }

        tInsNode->EventState = EventState;
        tInsNode->c_MsgProcess = *c_MsgProcess;
        if(findProcess(tInsNode,*tInsNodeHead)){
                OspLog(LOG_LVL_ERROR,"[regProcess]node already registered\n");
                return;
        }

        list_add(&tInsNode->tListHead,*tInsNodeHead);
}

static void delInsNode(struct list_head ** tInsNodeHead){

        struct list_head *insHead,*templist;
        TInsNode *tnInsNode;

        list_for_each_safe(insHead,templist,&*tInsNodeHead){
                tnInsNode = list_entry(insHead,TInsNode,tListHead);
                list_del(&tnInsNode->tListHead);
                delete tnInsNode;
        }

        INIT_LIST_HEAD(*tInsNodeHead);
}

void GuiInstance::InstanceEntry(CMessage *const pMsg){

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        msgProcess c_MsgProcess;
        TInsNode tInsNode;

        tInsNode.EventState = MAKEESTATE(curState,curEvent);
        tInsNode.c_MsgProcess = 
        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry]msg is NULL\n");
                return;
        }

        if(findProcess())

}

