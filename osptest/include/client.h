#include"osp.h"
#include"commondemo.h"


#define RUNNING_STATE                 1
#define IDLE_STATE                    0
#define OSP_AGENT_CLIENT_PORT         20001
#define EV_CLIENT_TEST_BGN           (u16)0x1111
#define SERVER_CONNECT_TEST          (EV_CLIENT_TEST_BGN+1)
#define CLIENT_ENTRY                (EV_CLIENT_TEST_BGN+2)
#define CLIENT_APP_ID                (u16)3
#define MAX_MSG_WAITING              (u32)512
#define CLIENT_APP_PRI               (u8)80
#define SIGN_STATUS_IN                1
#define SIGN_STATUS_OUT               0
#define AUTHORIZATION_NAME_SIZE       20

#define BUFFER_SIZE                   MAX_MSG_LEN
#define MAX_FILE_NAME_LENGTH         200


extern void test_file_send();
typedef struct tagSinInfo{
        s8 g_Username[AUTHORIZATION_NAME_SIZE];
        s8 g_Passwd[AUTHORIZATION_NAME_SIZE];
}TSinInfo;

class CCInstance : public CInstance{
private:
        u32         m_dwdstNode;
        u32         wDisInsID;
        SEMHANDLE   m_sem;
        u8          file_name_path[MAX_FILE_NAME_LENGTH];

        u8          buffer[BUFFER_SIZE];
private:
        void InstanceEntry(CMessage *const);
public:
        CCInstance(): m_dwdstNode(0),wDisInsID(0){
                OspSemBCreate(&m_sem);
                memset(file_name_path,0,sizeof(u8)*MAX_FILE_NAME_LENGTH);
                memset(buffer,0,sizeof(u8)*BUFFER_SIZE);
        }
        ~CCInstance(){
                OspSemDelete(m_sem);
        }
        u32 GetDstNode();
        void FileSendCmd2Client();
};

typedef zTemplate<CCInstance,MAX_INS_NUM,CAppNoData,MAX_ALIAS_LENGTH> CCApp;