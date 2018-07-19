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

#if 1
#define BUFFER_SIZE                   (u16)(MAX_MSG_LEN >> 1)
#else
#define BUFFER_SIZE                   (u16)1
#endif
#define MAX_FILE_NAME_LENGTH         200
#define CLIENT_INSTANCE_NUM          1

#define CLIENT_INSTANCE_ID           1


#define SEND_REMOVE                    (EV_CLIENT_TEST_BGN+17)
#define SEND_CANCEL                    (EV_CLIENT_TEST_BGN+18)
#define FILE_REMOVE_ACK                (EV_CLIENT_TEST_BGN+19)
#define FILE_CANCEL_ACK                (EV_CLIENT_TEST_BGN+20)

#define SEND_REMOVE_CMD                (EV_CLIENT_TEST_BGN+21)
#define SEND_CANCEL_CMD                (EV_CLIENT_TEST_BGN+22)

#define FILE_GO_ON_CMD                 (EV_CLIENT_TEST_BGN+23)
#define FILE_GO_ON                     (EV_CLIENT_TEST_BGN+24)
#define FILE_GO_ON_ACK                 (EV_CLIENT_TEST_BGN+25)

#define FILE_UPLOAD_DAEMON_CMD         (EV_CLIENT_TEST_BGN+26)


typedef struct tagSinInfo{
        s8 g_Username[AUTHORIZATION_NAME_SIZE];
        s8 g_Passwd[AUTHORIZATION_NAME_SIZE];
}TSinInfo;

typedef enum tagEM_FILE_STATUS{
                GO_ON_SEND       = 0,
                RECEIVE_CANCEL   = 1,
                RECEIVE_REMOVE   = 2,
                CANCELED         = 3,
                REMOVED          = 4,
                FINISHED         = 5
}EM_FILE_STATUS;


class CCInstance : public CInstance{

public:
        typedef void (CCInstance::*MsgProcess)(CMessage *const pMsg);
private:

        typedef struct tagCmdNode{
                u32         EventState;
                CCInstance::MsgProcess  c_MsgProcess;
                struct      tagCmdNode *next;
        }tCmdNode;


        u32         m_dwDisInsID;
        u8          file_name_path[MAX_FILE_NAME_LENGTH];

        s8          buffer[BUFFER_SIZE];
        u32         m_wFileSize;           //因为fseek返回值为int，实际最大值为2G,fopen也有这样的限制，设置
                                           //_FILE_OFFSET_BITS == 64或者使用-D_FILE_OFFSET_BITS = 64可能会
                                           //增加限制值,64位系统不存在这样的问题。
        u32         m_wUploadFileSize;
        EM_FILE_STATUS emFileStatus;
private:
        void InstanceEntry(CMessage *const);
        void DaemonInstanceEntry(CMessage *const,CApp*);
        tCmdNode *m_tCmdChain;
        tCmdNode *m_tCmdDaemonChain;
        FILE *file;
        bool m_bSignFlag;
        bool m_bConnectedFlag;
public:
        CCInstance(): m_dwDisInsID(0),file(NULL),m_wFileSize(0)
                     ,m_wUploadFileSize(0),m_bSignFlag(false)
                      ,m_bConnectedFlag(false),m_tCmdChain(NULL)
                      ,m_tCmdDaemonChain(NULL),emFileStatus(GO_ON_SEND){
                memset(file_name_path,0,sizeof(u8)*MAX_FILE_NAME_LENGTH);
                memset(buffer,0,sizeof(u8)*BUFFER_SIZE);
                MsgProcessInit();
        }
        ~CCInstance(){
                NodeChainEnd();
        }
        void MsgProcessInit();
        void NodeChainEnd();
        bool RegMsgProFun(u32,MsgProcess,tCmdNode**);
        bool FindProcess(u32,MsgProcess*,tCmdNode*);


        //注册处理函数
        void ClientEntry(CMessage* const);
        void SignInCmd(CMessage* const);
        void SignInAck(CMessage* const);
        void SignOutCmd(CMessage* const);
        void SignOutAck(CMessage* const);
        void FileNameAck(CMessage* const);

        void FileUploadCmd(CMessage* const);

        void FileUploadAck(CMessage* const);
        void FileFinishAck(CMessage* const);

        void RemoveCmd(CMessage* const);
        void CancelCmd(CMessage* const);
        void FileCancelAck(CMessage* const);
        void FileRemoveAck(CMessage* const);
        void FileGoOnCmd(CMessage* const);
        void FileGoOnAck(CMessage* const);


};

typedef zTemplate<CCInstance,CLIENT_INSTANCE_NUM,CAppNoData,MAX_ALIAS_LENGTH> CCApp;
