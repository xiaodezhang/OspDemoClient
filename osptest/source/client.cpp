#include"osp.h"
#include"client.h"

CCApp g_cCApp;
static u8 CancleFlag = 0;

#define MAKEESTATE(state,event) ((u32)(event<< 4 +state))

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
        test_file_send();
        while(1)
                OspDelay(100);

        OspQuit();
        return 0;
}


void test_file_send(){
        char file_name[] = "test_file_name";

        ::OspPost(MAKEIID(CLIENT_APP_ID,CInstance::EACH),FILE_NAME_SEND_CMD,
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

static tCmdNode *CmdChain = NULL;

void CCInstance::ClientEntry(CMessage *const pMsg){

printf("client entry\n");
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
}

void CCInstance::MsgProcessInit(){

#if 1
        RegMsgProFun(MAKEESTATE(IDLE_STATE,CLIENT_ENTRY),&CCInstance::ClientEntry);
#endif
}

void CCInstance::NodeChainEnd(){

        tCmdNode * Node;

        Node = CmdChain;
        if(!Node)
                return;
        while(Node->next){
                free(Node);
                Node = Node->next;
        }
}

bool CCInstance::RegMsgProFun(u32 EventState,MsgProcess c_MsgProcess){

        tCmdNode *Node,*NewNode;

        Node = CmdChain;

        if(!(NewNode = (tCmdNode*)malloc(sizeof(tCmdNode)))){
                OspLog(LOG_LVL_ERROR,"[RegMsgProFun] node malloc error\n");
                return false;
        }

        NewNode->EventState = EventState;
        NewNode->c_MsgProcess = c_MsgProcess;

        if(!CmdChain){
                CmdChain = NewNode;
                OspLog(SYS_LOG_LEVEL,"cmd chain init \n");
                return true;
        }

        while(Node){
                if(Node->EventState == EventState){
                        OspLog(LOG_LVL_ERROR,"[RegMsgProFun] node already in \n");
                        printf("[RegMsgProFun] node already in \n");
                        return false;
                }
                Node = Node->next;
        }
        Node = NewNode;
        Node->next = NULL;
}

bool CCInstance::FindProcess(u32 EventState,MsgProcess*c_MsgProcess){

        tCmdNode *Node;

        Node = CmdChain;
        if(!CmdChain){
                OspLog(LOG_LVL_ERROR,"[FindProcess] Node Chain is NULL\n");
                printf("[FindProcess] Node Chain is NULL\n");
                return false;
        }
        while(Node){
                printf("node eventstate:%ld\n",Node->EventState);
                printf("eventstate:%ld\n",EventState);
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

        printf("client instance entry\n");
        if(NULL == pMsg){
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
        }

        u32 curState = CurState();
        u16 curEvent = pMsg->event;
        MsgProcess c_MsgProcess;


#if 1
        if(FindProcess(MAKEESTATE(curState,curEvent),&c_MsgProcess)){
                printf("find the EState\n");
                (this->*c_MsgProcess)(pMsg);
        }else{
                OspLog(LOG_LVL_ERROR,"[InstanceEntry] can not find the EState\n");
                printf("[InstanceEntry] can not find the EState\n");
        }
#else


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
                                  //TODO,应该需要增加其他信息，目前是无法定位的，也有可能log函数自带定位
                                  //，需要确认
                                  OspLog(SYS_LOG_LEVEL,"sign in successfully\n");
                                  OspPrintf(1,1,"sign in successfully\n");
                                  printf("sign in successfully\n");

                                  //测试登出
                                  //TODO
                                  post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_OUT,NULL,0,m_dwdstNode);
//                                  post(wDisInsID,SIGN_OUT,NULL,0,m_dwdstNode);
					
                                  printf("send sign out\n");
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
                             break;
                     }
                }
                        break;
                case RUNNING_STATE:
                     switch(curEvent){
                             //TODO：登出命令由GUI发送，根据appid,
                             case SIGN_OUT_CMD:{
                                  post(MAKEIID(SERVER_APP_ID,CInstance::DAEMON),SIGN_OUT,NULL,0,m_dwdstNode);
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
                             case FILE_NAME_SEND_CMD:{
                                  if(!pMsg->content || pMsg->length <= 0){
                                           OspLog(LOG_LVL_ERROR,"[InstanceEntry] pMsg is NULL\n");
                                           OspPrintf(1,0,"[InstanceEntry] pMsg is NULL\n");
                                           break;
                                  }
                                  strcpy((char*)file_name_path,(const char*)pMsg->content);
                                  post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_NAME_SEND
                                                 ,pMsg->content,pMsg->length,GetDstNode());

                             }
                             break;
                             case FILE_NAME_ACK:{
                                  FILE* file;
                                  size_t buffer_size;


                                  wDisInsID = pMsg->srcid;
                                  if(!(file = fopen((const char*)file_name_path,"rb"))){
                                          printf("open file error\n");
                                          break;
                                  }

#if 0
                                  buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file);
                                  if(buffer_size > 0){
                                        post(MAKEIID(SERVER_APP_ID,CInstance::PENDING),FILE_UPLOAD
                                                   ,buffer,buffer_size,GetDstNode());

                                  }
#endif

                                  while((buffer_size = fread(buffer,1,sizeof(char)*BUFFER_SIZE,file)) > 0){
                                          //TODO:增加中断变量,信号量控制,记录文件读取位置
                                          //大文件传送测试
                                          post(wDisInsID,FILE_UPLOAD,buffer,buffer_size,m_dwdstNode);
                                          OspSemTake(m_sem);
                                          if(CancleFlag){
                                                  OspSemGive(m_sem);
                                                  break;
                                          }
                                          OspSemGive(m_sem);
                                  }
                                  fclose(file);

                             }
                             break;
                     }
                        break;
                default:
                        break;
        }

        OspPrintf(1,0,"instance entry\n");

#endif
}

