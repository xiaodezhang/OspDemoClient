#ifndef _DEMO_GUI_
#define _DEMO_GUI_

#define DEMO_GUI_LISTEN_PORT                    5000
#define GUI_APP_ID                              (u16)4
#define GUI_APP_PRI                             (u8)80
#define CREATE_TCP_NODE_TIMES     20

#define GUI_MSG_BASE                            (u16)0xf111

#define GUI_SIGN_IN_ACK                         (GUI_MSG_BASE+1)
#define GUI_SIGN_OUT_ACK                        (GUI_MSG_BASE+2)
#define GUI_FILE_SIZE_ACK                       (GUI_MSG_BASE+3)
#define GUI_UPLOAD_FILE_SIZE_ACK                (GUI_MSG_BASE+4)
#define GUI_DISCONNECT                          (GUI_MSG_BASE+5)
#define GUI_FILE_UPLOAD_ACK                     (GUI_MSG_BASE+6)
#define GUI_FILE_FINISHED_ACK                   (GUI_MSG_BASE+7)
#define GUI_FILE_CANCEL_ACK                     (GUI_MSG_BASE+8)
#define GUI_FILE_REMOVE_ACK                     (GUI_MSG_BASE+9)

class GuiInstance : public CInstance{

private:
        u8          m_byServerIp[MAX_IP_LENGTH];
        u16         m_wServerPort;
private:
        void InstanceEntry(CMessage *const);
        void DaemonInstanceEntry(CMessage *const,CApp*);
public:
        GuiInstance(): m_wServerPort(SERVER_PORT){
                  memcpy(m_byServerIp,SERVER_IP,sizeof(SERVER_IP));
        }
        ~GuiInstance(){
        }
};

typedef zTemplate<GuiInstance,MAX_INS_NUM,CAppNoData,MAX_ALIAS_LENGTH> GuiApp;

#endif

