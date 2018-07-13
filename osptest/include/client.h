#include"osp.h"
#include"commondemo.h"


#define RUNNING_STATE                 1
#define IDLE_STATE                    0
#define OSP_AGENT_CLIENT_PORT         20001
#define EV_CLIENT_TEST_BGN           (u16)0x1111
#define SERVER_CONNECT_TEST          (EV_CLIENT_TEST_BGN+1)
#define CLIENT_ENTRY                 (EV_CLIENT_TEST_BGN+2)
#define CLIENT_APP_ID                (u16)3
#define MAX_MSG_WAITING              (u32)512
#define CLIENT_APP_PRI               (u8)80
#define SIGN_STATUS_IN                1
#define SIGN_STATUS_OUT               0
#define AUTHORIZATION_NAME_SIZE       20

#define BUFFER_SIZE                   MAX_MSG_LEN
#define MAX_FILE_NAME_LENGTH         200
#define CLIENT_INSTANCE_NUM          1

#define CLIENT_INSTANCE_ID           1


extern void test_file_send();
typedef struct tagSinInfo{
        s8 g_Username[AUTHORIZATION_NAME_SIZE];
        s8 g_Passwd[AUTHORIZATION_NAME_SIZE];
}TSinInfo;


class CCInstance : public CInstance{

public:
typedef void (CCInstance::*MsgProcess)(CMessage *const pMsg);
private:

typedef struct tagCmdNode{
        u32         EventState;
        CCInstance::MsgProcess  c_MsgProcess;
        struct      tagCmdNode *next;
}tCmdNode;


        u32         m_dwdstNode;
        u32         m_dwDisInsID;
        SEMHANDLE   m_sem;
        u8          file_name_path[MAX_FILE_NAME_LENGTH];

        s8          buffer[BUFFER_SIZE];
private:
        void InstanceEntry(CMessage *const);
        void DaemonInstanceEntry(CMessage *const,CApp*);
        tCmdNode *m_tCmdChain;
        tCmdNode *m_tCmdDaemonChain;
public:
        CCInstance(): m_dwdstNode(0),m_dwDisInsID(0){
                m_tCmdChain = NULL;
                m_tCmdDaemonChain = NULL;
                OspSemBCreate(&m_sem);
                memset(file_name_path,0,sizeof(u8)*MAX_FILE_NAME_LENGTH);
                memset(buffer,0,sizeof(u8)*BUFFER_SIZE);
                MsgProcessInit();
        }
        ~CCInstance(){
                OspSemDelete(m_sem);
                NodeChainEnd();
        }
        u32 GetDstNode();
        void FileSendCmd2Client();
        void MsgProcessInit();
        void NodeChainEnd();
        bool RegMsgProFun(u32,MsgProcess,tCmdNode**);
        bool FindProcess(u32,MsgProcess*,tCmdNode*);


        //注册处理函数
        void ClientEntry(CMessage*const);
        void SignInAck(CMessage*const);
        void SignOutCmd(CMessage*const);
        void SignOutAck(CMessage*const);
        void FileNameSendCmd(CMessage*const);
        void FileNameAck(CMessage*const);
};

typedef zTemplate<CCInstance,CLIENT_INSTANCE_NUM,CAppNoData,MAX_ALIAS_LENGTH> CCApp;
