#ifndef _DEMO_GUI_
#define _DEMO_GUI_

class GuiInstance : public CInstance{

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
        u8          m_byServerIp[MAX_IP_LENGTH];
        u16         m_wServerPort;
#if MULTY_APP
        bool        m_bConnectedFlag;    //避免使用线程锁，将全局变量改为成员变量
        bool        m_bSignFlag;         //避免使用线程锁，将全局变量改为成员变量
#endif
private:
        void InstanceEntry(CMessage *const);
        void DaemonInstanceEntry(CMessage *const,CApp*);
        tCmdNode *m_tCmdChain;
        tCmdNode *m_tCmdDaemonChain;
        FILE *file;
public:
        CCInstance(): m_dwDisInsID(0),file(NULL),m_wFileSize(0)
                     ,m_wUploadFileSize(0)
#if MULTY_APP
                     ,m_bConnectedFlag(false)
                     ,m_bSignFlag(false)
#endif
                     ,m_tCmdChain(NULL)
                     ,m_tCmdDaemonChain(NULL),emFileStatus(STATUS_INIT)
                     ,m_wServerPort(SERVER_PORT){
                memset(file_name_path,0,sizeof(u8)*MAX_FILE_NAME_LENGTH);
                memset(buffer,0,sizeof(u8)*BUFFER_SIZE);
                memcpy(m_byServerIp,SERVER_IP,sizeof(SERVER_IP));
                MsgProcessInit();
        }
        ~CCInstance(){
                NodeChainEnd();
                if(file){
                        fclose(file);
                }
        }
        void MsgProcessInit();
        void NodeChainEnd();
        bool RegMsgProFun(u32,MsgProcess,tCmdNode**);
        bool FindProcess(u32,MsgProcess*,tCmdNode*);


        //注册处理函数
        void SignInCmd(CMessage* const);
        void SignInAck(CMessage* const);
        void SignOutCmd(CMessage* const);
        void SignOutAck(CMessage* const);



        void FileReceiveUploadAck(CMessage* const);

        void FileUploadCmd(CMessage* const);

        void FileUploadAck(CMessage* const);
        void FileFinishAck(CMessage* const);

        void RemoveCmd(CMessage* const);
        void CancelCmd(CMessage* const);
        void FileCancelAck(CMessage* const);
        void FileRemoveAck(CMessage* const);
        void FileGoOnCmd(CMessage* const);
        void FileGoOnAck(CMessage* const);
        void FileUploadCmdDeal(CMessage* const);
        void SendCancelCmdDeal(CMessage* const);
        void CancelCmdDeal(CMessage* const);
        void FileGoOnCmdDeal(CMessage* const);
        void RemoveCmdDeal(CMessage* const);
        void StableRemoveCmdDeal(CMessage* const);

#if MULTY_APP
        void GetDisconnect(CMessage* const);
        void notifyConnect(CMessage* const);
        void GetMyConnect(CMessage* const);
        void notifySigned(CMessage* const);
        void notifyDissigned(CMessage* const);
        void GetDissigned(CMessage* const);
        void GetSigned(CMessage* const);
#endif
        //断链检测处理函数
        void notifyDisconnect(CMessage* const);


};

typedef zTemplate<GuiInstance,MAX_INS_NUM,CAppNoData,MAX_ALIAS_LENGTH> GuiApp;

#endif

