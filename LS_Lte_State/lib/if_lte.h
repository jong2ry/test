#ifndef _IF_LTE_H
#define _IF_LTE_H

#ifdef HAVE_LTE_INTERFACE

#define LTE_BUF_LENGTH                  512
#define MAX_BUF_LENGTH                  64
#define MAX_TOKEN_BUF_LENGTH            MAX_BUF_LENGTH

#define LTE_IF_TYPE_PPP                 1
#define LTE_IF_TYPE_LTE                 2

//----------------------------------------------------------------------------
// main struct....!!!!
//----------------------------------------------------------------------------
typedef struct ZEBOS_LTE_MODULE
{
    int state;                          // 연결상태
    char state_name[MAX_BUF_LENGTH];    // 연결상태 이름(제조사마다 다름)
    char name[MAX_BUF_LENGTH];          // 네트워크 이름 (KT / SKT / LGU+)
    char type[MAX_BUF_LENGTH];          // LTE / WCDMA
    int level;                          // 신호레벨(0% ~ 100%)
    int rssi;                           // 신호세기
    int rsrp;                           // 신호세기
    int rsrq;                           // 신호세기
    int sinr;                           // 신호세기
    char lgu_uicc[MAX_BUF_LENGTH];      // LGU USIM 상태
    char number[MAX_BUF_LENGTH];        // 전화번호
    char number2[MAX_BUF_LENGTH];       // 전화번호2
    char info[MAX_BUF_LENGTH];          // 모듈정보
    char manufacturer[MAX_BUF_LENGTH];  // 제조사
    char imei[MAX_BUF_LENGTH];          // IMEI 정보
    char if_type[MAX_BUF_LENGTH];       // 모듈의 인터페이스 타입
    char ip_addr[MAX_BUF_LENGTH];       // 모듈의 IP Address
    char apn_name[MAX_BUF_LENGTH];      // (mobile / internet / m2m-router)
    char ruimid[MAX_BUF_LENGTH];        // Anydata 모듈의 RUIMID 값
}__attribute__((packed)) ZEBOS_LTE_MODULE_t;

#define ZEBOS_NSM_LTE_KEY   2214


//----------------------------------------------------------------------------
// function list
//----------------------------------------------------------------------------
void lte_set_sysLED(int cmd);
void lte_set_sysLTE(int cmd);

void lte_reset ();
void lte_check_factoryreset (void);

int lte_get_product (void);
int lte_get_module_info(ZEBOS_LTE_MODULE_t *module);

void lte_show_status(struct cli *cli, ZEBOS_LTE_MODULE_t *mod);
void lte_show_status_detail(struct cli *cli);

void lte_send_sms(struct cli *cli, char *pNumber, char *pMsg);
void lte_send_serial(char *cmd, char *pbuf);

int lte_ota_from_skt(struct cli *cli);
int lte_ota_from_kt(struct cli *cli);

int lte_change_apn(int type, char *apn_url);
void lte_show_apn(struct cli *cli);
void lte_show_apn_detail(struct cli *cli);

int lte_is_network_service_available (void);
int lte_is_available (void);
int lte_is_driver (char *pCmd);
int lte_get_level (void);
int lte_enable_rmnet (void);


//----------------------------------------------------------------------------
// AT commmand
//----------------------------------------------------------------------------
#define TTY_AMT                         "/dev/ttyAMT0"      // AMTelecom
#define TTY_KNI0                        "/dev/ttyKNI0"      // Kernel-I
#define TTY_KNI1                        "/dev/ttyKNI1"      // Kernel-I
#define TTY_MEC                         "/dev/ttyMEC0"      // MobileECO
#define TTY_ANY                         "/dev/ttyUSB103"    // Anydata
#define TTY_NT                          "/dev/ttyNT0"       // NTmore
#define TTY_LGM2M_TYPE1                 "/dev/GCT-ATC0"     // LGM2M TYPE 1
#define TTY_NTLM200Q                    "/dev/ttyNTLM200Q0" // NTmore NTLM200Q
#define TTY_L300S                       "/dev/ttyL300S0"    // Pantech PM-L300S
#define TTY_PEGASUS                     "/dev/ttyPEG0"      // LG PEGASUS 
#define TTY_TM400                       "/dev/ttyTM4000"    // Teladin TM400

#define LTEIF_LGU_MOBILE                "/media/disk0/.lgu_mobile"    // LGU+ Network Type Flag file
#define LTEIF_LGU_INTERNET              "/media/disk0/.lgu_internet"  // LGU+ Network Type Flag file
#define LTEIF_LGU_M2M                   "/media/disk0/.lgu_m2m"       // LGU+ Network Type Flag file

#define AT_NO_WAIT                      0
#define AT_WAIT                         1

#define PROBE_MAX_COUNT                 10

enum {
  PRODUCT_NONE= 0,
  PRODUCT_AMT,              // AMTelecom
  PRODUCT_KNI,              // CS
  PRODUCT_MEC,              // MobileECO
  PRODUCT_ANYDATA,          // Anydata
  PRODUCT_LGM2M_TYPE1,      // LG M2M TYPE 1
  PRODUCT_LGM2M_TYPE2,      // LG M2M TYPE 2
  PRODUCT_NT,               // NTmore
  PRODUCT_NTLM200Q,         // NTmore 200Q
  PRODUCT_L300S,            // Pantech PM-L300S
  PRODUCT_PEGASUS,          // LG M2M PEGASUS
  PRODUCT_TM400,            // Teladin TM400
};

enum {
    LTE_ERROR = 0,
    LTE_OK = !(LTE_ERROR)
};

enum {
    DISCONNECT = -1,
    CONNECT = 0
};

enum {
    OTA_TYPE_KT = 0,
    OTA_TYPE_SKT = 1
};

enum {
    APN_TYPE_NONE = 0,
    APN_TYPE_KT,
    APN_TYPE_SKT,
    APN_TYPE_LGU,
    APN_TYPE_DEFAULT,
    APN_TYPE_USER
};


#define AMT_STATE_INDEX                 3
#define AMT_NETTYPE_INDEX               7
#define AMT_RSSI_INDEX                  2
#define AMT_GMM_INDEX                   1
#define AMT_IMEI_INDEX                  1
#define AMT_CNUM_INDEX                  2

#define LGM2M_TYPE2_STATE_INDEX         1
#define LGM2M_TYPE2_LGTRSSI_INDEX       1
#define LGM2M_TYPE2_LGTMDN_INDEX        1
#define LGM2M_TYPE2_IMEI_INDEX          0

#define KNI_CGMR_INDEX                  0
#define KNI_CNUM_INDEX                  1
#define KNI_IMEI_INDEX                  0


//----------------------------------------------------------------------------
// LGU+ network type
//----------------------------------------------------------------------------
enum {
  LTEIF_LGU_MOBILE_NETWORK = 0,
  LTEIF_LGU_INTERNET_NETWORK,
  LTEIF_LGU_M2M_NETWORK
};

//----------------------------------------------------------------------------
// LTE module state definition
//----------------------------------------------------------------------------
typedef struct _lte_state
{
    int state;
    char *name;
} LTE_STATE;


#define _CM_STATE_LOADING                           0x00
#define _CM_STATE_MODEM_SEARCH                      0x01
#define _CM_STATE_MODEM_NOTFOUND                    0x02
#define _CM_STATE_MODEM_INIT                        0x03
#define _CM_STATE_MODEM_INIT_FAIL                   0x04
#define _CM_STATE_UICC_CHECK                        0x05
#define _CM_STATE_UICC_CHECK_FAIL                   0x06
#define _CM_STATE_INVALID_UICC                      0x07
#define _CM_STATE_PIN_CHECK                         0x08
#define _CM_STATE_PIN_CHECK_FAIL                    0x09
#define _CM_STATE_NETWORK_SCAN                      0x0A
#define _CM_STATE_NETWORK_NOTFOUND                  0x0B
#define _CM_STATE_CONNECT_READY                     0x0C
#define _CM_STATE_CONNECTING                        0x0D
#define _CM_STATE_CONNECT_CANCEL                    0x0E
#define _CM_STATE_CONNECT_FAIL                      0x0F
#define _CM_STATE_CONNECTED                         0x10
#define _CM_STATE_DISCONNECTING                     0x11
#define _CM_STATE_DISCONNECTED                      0x12
#define _CM_STATE_OUT_OF_SERVICE                    0x13
#define _CM_STATE_SLEEP                             0x14
#define _CM_STATE_AIRPLANE                          0x15
#define _CM_STATE_IP_ALLOC                          0x16
#define _CM_STATE_ERROR                             0x17
#define _CM_STATE_BIP_OPEN                          0x18
#define _CM_STATE_UNKNOWN                           0xFF


#define _UICC_CHK_NONE                              0x00
#define _UICC_CHK_SUCCESS                           0x01
#define _UICC_NOT_LGUPLUS                           0x02
#define _UICC_CHK_FAIL                              0x03
#define _UICC_CHK_SUCCESS_NO_PIN                    0x04
#define _UICC_CHK_SUCCESS_PIN_NOT_INITIALIZED       0x05
#define _UICC_CHK_SUCCESS_PIN_REQUIRE               0x06
#define _UICC_CHK_SUCCESS_PIN_ENABLED_VERIFIED      0x07
#define _UICC_CHK_SUCCESS_PIN_DISABLED              0x08
#define _UICC_CHK_SUCCESS_PIN_PIN_BLOCKED           0x09
#define _UICC_CHK_SUCCESS_PIN_PERMANENTLY_BLOCKED   0x0A
#define _UICC_CHK_SUCCESS_PIN_CHK_FAIL              0x0B
#define _UICC_CHK_UICC_REMOVE                       0x0C
#define _UICC_BIP_OPENING_REQUIRE                   0x0D
#define _UICC_BIP_OPENING_PROGRESS                  0x0E
#define _UICC_BIP_OPENING_SUCCESS                   0x0F
#define _UICC_BIP_DETACH_RSP                        0x10
#define _UICC_BIP_UICC_DEACTIVE                     0x11
#define _UICC_BIP_OPENING_FAIL                      0x12



#define  LTE_CM_STATE_KEY       6281
#define  LTE_CM_RF_DEBUG_KEY    6287


/**
  Write : LTECM
  Read  : bmon, WEB U
**/
typedef struct LTE_CM_STATE_SHM
{
   char TimeStamp[6+1]; // HHMMSS
   unsigned char CmState;
   char  CmStateStr[255+1];
   unsigned char UiccState;
   char UiccStateStr[255+1];
   int PinRemainCnt;
   int PukRemainCnt;
   int Rssi;
   int Rsrp;
   int  EmmCause;
   unsigned char IMSI[20+1];
   unsigned char Rrc;  /* Connected or IDLE */
   unsigned char BIPState;      // aaaaa BIP
}__attribute__((packed)) LTE_CM_STATE_SHM_t;



typedef struct _CQI_CODE_STATISTICS_INFO
{
        unsigned short each_cqi_mode[16];
}__attribute__((packed)) CQI_CODE_STATISTICS_INFO, *PCQI_CODE_STATISTICS_INFO;

typedef struct _CQI_TYPE_STATISTICS_INFO
{
        CQI_CODE_STATISTICS_INFO cqi_type_aperiodic[5];
        CQI_CODE_STATISTICS_INFO cqi_type_periodic[4];
}__attribute__((packed)) CQI_TYPE_STATISTICS_INFO, *PCQI_TYPE_STATISTICS_INFO;

typedef struct _CQI_REPORT_RSP_INFO
{
        CQI_TYPE_STATISTICS_INFO cw1_cqi;
        CQI_TYPE_STATISTICS_INFO cw2_cqi;
}__attribute__((packed)) CQI_REPORT_RSP_INFO, *PCQI_REPORT_RSP_INFO;


typedef struct LTE_RF_DEBUG_SHM
{
    char TimeStamp[6+1]; // HHMMSS

  // TX POWER
  signed int pusch;  // -55 ~ +23 (dBm unit)
  signed int pucch;  // -55 ~ +23 (dBm unit)
  signed int prach;  // -55 ~ +23 (dBm unit)

  // UL/DL THROUGHPUT
  unsigned int cur_ul;  // Current sent data rate (bytes/sec)
  unsigned int cur_dl;  // Current received data rate (bytes/sec)
  unsigned int max_ul;  // Maximum sent data rate (bytes/sec)
  unsigned int max_dl;  // Maximum data rate (bytes/sec)
  unsigned int total_ul; // Total sent data (in bytes)
  unsigned int total_dl; // Total received data (in bytes)

  // PDSCH BLER
  unsigned short total_cw1;
  unsigned short total_cw2;
  unsigned short error_cw1;
  unsigned short error_cw2;

  // SERVING CELL
  unsigned short physical_cell_id;
  unsigned char tac[2];
  unsigned int cgi;
  short rssi;
  short rsrp;
  short rsrq;
  short sinr;
  short tx_power;
  unsigned int dl_freq;
  unsigned short earfcn;
  unsigned int band;
  unsigned int bandwidth;
  unsigned char state;
  unsigned char ecgi[7];

  // CQI Stastics
  CQI_REPORT_RSP_INFO cqi_stastics;

  // MCS UL Stastics
  unsigned short mcs[32];
  // MCS DL Stastics
  unsigned short cw1_mcs[32];
  unsigned short cw2_mcs[32];

}__attribute__((packed)) LTE_RF_DEBUG_SHM_t;


//----------------------------------------------------------------------------
// IOCTL COMMAND
//----------------------------------------------------------------------------
#define CMD_SYSLTE_PUP            0
#define CMD_SYSLTE_PDN            1
#define CMD_SYSLTE_RESET          2
#define CMD_SYSM2M_DI_READ        3
#define CMD_SYSM2M_FAC_READ       4

#define CMD_SIG_LED_PDN           0
#define CMD_SIG_LED_PUP           1
#define CMD_LTE_LED_PDN           2
#define CMD_LTE_LED_PUP           3

//----------------------------------------------------------------------------
// LTE M2M definition
//----------------------------------------------------------------------------
#define M2M_VENDOR              "1d74"
#define M2M_TYPE1_PRODUCT       "2300"
#define M2M_TYPE2_PRODUCT       "2600"
#define M2M_GET_M2MPCM_CMD       "ps -ef |grep m2mp-cm | awk '{print $8}'|grep -c m2mp-cm"

#define LGM2M_TYPE2_SOCK_IP     "10.0.0.1"
#define LGM2M_TYPE2_SOCK_PORT   1040
#define LGM2M_TYPE2_HEADER_SIZE 4

#define CHK_CDC_ETHER_DRV   "lsmod |grep cdc_ether |awk '{print $1}' |grep cdc_ether"
#define CHK_CDC_ACM_DRV     "lsmod |grep cdc_acm |awk '{print $1}' |grep cdc_acm"
#define CHK_RNDIS_HOST_DRV  "lsmod |grep rndis_host |awk '{print $1}' |grep rndis_host"
#define CHK_GDMULTE_DRV     "lsmod |grep gdmulte |awk '{print $1}' |grep gdmulte"
#define CHK_GDMTTY_DRV      "lsmod |grep gdmtty |awk '{print $1}' |grep gdmtty"
#define CHK_M2M_SDK         "ps -ef |grep m2mp-cm |awk '{print $8}'|grep m2mp-cm"



//----------------------------------------------------------------------------
// M2M external signal
//----------------------------------------------------------------------------
#define LTE_M2M_CHK_INTERVAL            1
#define LTE_M2M_EXT_SIGNAL_READ_CMD     "cat /proc/nexg/sysm2m_ext_signal"
#define LTE_M2M_EXT_SIGNAL_NONE         "none"
#define LTE_M2M_EXT_SIGNAL_CONNECT      "connect"
#define LTE_M2M_EXT_SIGNAL_DISCONNECT   "disconnect"

/** external signal state */
enum {
    LTE_M2M_EXT_STATE_ERROR = -1,
    LTE_M2M_EXT_STATE_NONE = 0,
    LTE_M2M_EXT_STATE_CONNECT,
    LTE_M2M_EXT_STATE_DISCONNECT
};

/** external signal struct */
typedef struct _ext_state{
    int prev_state;
    int now_state;
}EXT_STATE;
EXT_STATE ext_state;



//----------------------------------------------------------------------------
// KT IOT 정보
//----------------------------------------------------------------------------
enum {
  KNI_SYSINFO_DUMMY= 0,
  KNI_SYSINFO_SRV_STATUS,
  KNI_SYSINFO_SYS_MODE,
  KNI_SYSINFO_ROAM_STATUS,
  KNI_SYSINFO_SIM,
  KNI_SYSINFO_VOICE_DOMAIN,
  KNI_SYSINFO_SMS_DOMAIN,
  KNI_SYSINFO_MCC,
  KNI_SYSINFO_MNC,
  KNI_SYSINFO_BAND,
  KNI_SYSINFO_CHANNEL,
  KNI_SYSINFO_RSSI,
  KNI_SYSINFO_RSCP,
  KNI_SYSINFO_BIT_ERR_RATE,
  KNI_SYSINFO_ECIO,
  KNI_SYSINFO_RSRP,
  KNI_SYSINFO_RSRQ,
  KNI_SYSINFO_LAC,
  KNI_SYSINFO_TAC,
  KNI_SYSINFO_RAC,
  KNI_SYSINFO_CELL_ID,
  KNI_SYSINFO_EMM_STATE,
  KNI_SYSINFO_EMM_SUBSTATE,
  KNI_SYSINFO_EMM_CONNECTION,
  KNI_SYSINFO_EMM_CAUSE,
  KNI_SYSINFO_TIN,
  KNI_SYSINFO_RRC,
};

typedef struct kni_sysinfo
{
  int index;
  char contents[64];
} kni_sysinfo_t;

typedef struct kt_iot_kni_info
{
  char subject[32];
  char at_cmd[32];
  int  index;
  int  result;
  char contents[64];
  kni_sysinfo_t *list;
} kt_iot_kni_info_t;

// LG M2M PEGASUS 모듈의 APN을 표현하기 위하여 
// buf의 길이를 256으로 변경합니다. 
typedef struct lte_apn_info
{
  char short_apn[256];
  char long_apn[256];
} lte_apn_info_t;


// LTER Factoryreset definition
#define LTER_DI_CONNECT             0
#define LTER_DI_DISCONNECT          1
#define LTER_FAC_NONE               2
#define LTER_FAC_RESET              3

#define LTER_CONNECTION_TIMEOUT     600


//----------------------------------------------------------------------------
// VF403 PPP module control
//----------------------------------------------------------------------------
#define VF403_RESET_PPP_GPIO        3

#endif /* HAVE_LTE_INTERFACE */


#endif /* !_IF_LTE_H */

/*
 * ex: ts=8:sw=2:sts=2:et
 */
