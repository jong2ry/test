#include "pal.h"

#ifdef HAVE_LTE_INTERFACE
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"
#include "cli.h"
#include "lib/buffer.h"
#include "common/common_drv.h"
#include "if_lte.h"
#include "lib/lte/ntlm200q.h"
#include "lib/lte/serial.h" 

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof (A) / sizeof ((A)[0]))
#endif /* !ARRAY_SIZE */

//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
//#define DBG    console_print
#define DBG(x...)

LTE_STATE kni_state_list[] = {
  {0x00, "NO SERVICE"},
  {0x01, "LIMIT SERVICE"},
  {0x02, "IN SERVICE"},
  {0x03, "LIMITED REGIONAL SERVICE"},
  {0x04, "POWER SAVE OF DEEP SLEEP"},
};

kni_sysinfo_t sysmode_list[] = {
{0, "NO SRV"},
{1, "AMPS"},
{2, "CDMA"},
{3, "GSM"},
{4, "EVDO"},
{5, "WCDMA"},
{6, "GSP"},
{7, "GSM/WCDMA"},
{8, "WLAN"},
{9, "LTE"},
{10,"GSM/WCDMA/LTE"},
{11, "TDS"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t sim_list[] = {
{0, "SIM is not available"},
{1, "SIM is available" },
{-1, "UNKNOWN"},
};

kni_sysinfo_t band_list[] = {
{0, "CDMA US CELL"},
{1, "CDMA US PCS"},
{3, "CDMA JAPAN CELL"},
{4, "CDMA KOREAN CELL"},
{5, "CDMA CDMA 450M"},
{40, "GSM 450M"},
{41, "GSM 480M"},
{42, "GSM 750M"},
{43, "GSM 850M"},
{44, "E-GSM 900M"},
{45, "P-GSM 900M"},
{46, "R-GSM 900M"},
{47, "GSM DCS 1800M"},
{48, "GSM PCD 1800M"},
{80, "WCDMA_IMT 2100M"},
{81, "WCDMA US 1900M"},
{82, "WCDMA EU 1800M"},
{83, "WCDMA US 1700M"},
{84, "WCDMA US 850M"},
{85, "WCDMA JAPAN 800M"},
{86, "WCDMA EU 2600M"},
{87, "WCDMA EU 900"},
{88, "WCDMA JAPAN 1700M"},
{90, "WCDMA 1500M"},
{91, "WCDMA JAPAN 850M"},
{120, "LTE BAND 1"},
{121, "LTE BAND 2"},
{122, "LTE BAND 3"},
{123, "LTE BAND 4"},
{124, "LTE BAND 5"},
{125, "LTE BAND 6"},
{126, "LTE BAND 7"},
{127, "LTE BAND 8"},
{128, "LTE BAND 9"},
{129, "LTE BAND 10"},
{130, "LTE BAND 11"},
{131, "LTE BAND 12"},
{132, "LTE BAND 13"},
{133, "LTE BAND 14"},
{134, "LTE BAND 15"},
{135, "LTE BAND 16"},
{136, "LTE BAND 17"},
{137, "LTE BAND 18"},
{138, "LTE BAND 19"},
{139, "LTE BAND 20"},
{140, "LTE BAND 21"},
{141, "LTE BAND 22"},
{142, "LTE BAND 23"},
{143, "LTE BAND 24"},
{144, "LTE BAND 25"},
{145, "LTE BAND 26"},
{146, "LTE BAND 27"},
{147, "LTE BAND 28"},
{148, "LTE BAND 29"},
{149, "LTE BAND 30"},
{150, "LTE BAND 31"},
{151, "LTE BAND 32"},
{152, "LTE BAND 33"},
{153, "LTE BAND 34"},
{154, "LTE BAND 35"},
{155, "LTE BAND 36"},
{156, "LTE BAND 37"},
{157, "LTE BAND 38"},
{158, "LTE BAND 39"},
{159, "LTE BAND 40"},
{160, "LTE BAND 41"},
{161, "LTE BAND 42"},
{162, "LTE BAND 43"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t emm_list[] = {
{1, "EMM DEREGISTERED"},
{2, "EMM REGISTERED INITIATED"},
{3, "EMM REGISTERED"},
{4, "EMM TRACKING AREA UPDATING INITIATED"},
{5, "EMM SERVICE_REAUEST INITIATED"},
{6, "EMM DEREGISTERED INITIATED"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t emm_sub_1[] = {
{0, "EMM DEREGISTERED NO IMSI"},
{1, "EMM DEREGISTERED PLMN SEARCH"},
{2, "EMM DEREGISTERED ATTACH NEEDED"},
{3, "EMM DEREGISTERED NO CELL AVAILABLE"},
{4, "EMM DEREGISTERED ATTEMPTING_TO_ATTACH"},
{5, "EMM DEREGISTERED NORMAL SERVICE"},
{6, "EMM DEREGISTERED LIMITED SERVICE"},
{7, "EMM DEREGISTERED WAITING PDN CONN REQ"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t emm_sub_2[] = {
{0, "EMM WAITING FOR NW RESPONSE"},
{1, "EMM WAITING FOR ESM RESPONSE"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t emm_sub_3[] = {
{0, "EMM REGISTERED NORMAL SERVICE"},
{1, "EMM REGISTERED UPDATE NEEDED"},
{2, "EMM REGISTERED ATTEMPTING TO UPDATE"},
{3, "EMM REGISTERED NO CELL AVAILABLE"},
{4, "EMM REGISTERED PLMN SEARCH"},
{5, "EMM REGISTERED LIMITED SERVICE"},
{6, "EMM REGISTERED ATTEMPTING TO UPDATE MM"},
{7, "EMM REGISTERED IMSI DETACH INITIATED"},
{8, "EMM REGISTERED WAITING FOR ESM ISR STATUS"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t emm_connection[] = {
{0, "EMM IDLE STATE"},
{1, "EMM WAITING FOR RRC CONFIRMATION STATE"},
{2, "EMM CONNECTED STATE"},
{3, "EMM RELEASE RRC CONNECTION STATE"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t tin_list[] = {
{0, "NAS MM TIM P TMSI"},
{1, "NAS MM TIN GUTI"},
{2, "NAS MM TIM RAT RELATED TMSI"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t rrc_list[] = {
{0, "RRC ACTIVATED"},
{1, "RRC DEACTIVATED"},
{-1, "UNKNOWN"},
};

kni_sysinfo_t calldrop_list[] = {
{0, "Normal state"},
{1, "call drop state"},
{-1, "UNKNOWN"},
};


kt_iot_kni_info_t kni_list[] =
{
  /* subject      at_command       index                       result   content  list      */
  { "3G/LTE",         "at+sysinfo",    KNI_SYSINFO_SYS_MODE,       0,  "-", sysmode_list},
  { "BAND/FREQ",      "at+sysinfo",    KNI_SYSINFO_BAND,           0,  "-", band_list},
  { "RSSI",           "at+sysinfo",    KNI_SYSINFO_RSSI,           0,  "-", NULL},
  { "RSRP",           "at+sysinfo",    KNI_SYSINFO_RSRP,           0,  "-", NULL},
  { "RSRQ",           "at+sysinfo",    KNI_SYSINFO_RSRQ,           0,  "-", NULL},
  { "PLMN_MCC",       "at+sysinfo",    KNI_SYSINFO_MCC,            0,  "-", NULL},
  { "PLMN_MNC",       "at+sysinfo",    KNI_SYSINFO_MNC,            0,  "-", NULL},
  { "LAC",            "at+sysinfo",    KNI_SYSINFO_LAC,            0,  "-", NULL},
  { "RAC",            "at+sysinfo",    KNI_SYSINFO_RAC,            0,  "-", NULL},
  { "TAC",            "at+sysinfo",    KNI_SYSINFO_TAC,            0,  "-", NULL},
  { "EMM",            "at+sysinfo",    KNI_SYSINFO_EMM_STATE,      0,  "-", emm_list},
  { "EMM_CAUSE",      "at+sysinfo",    KNI_SYSINFO_EMM_CAUSE,      0,  "-", NULL},
  { "EMM_SUB",        "at+sysinfo",    KNI_SYSINFO_EMM_SUBSTATE,   0,  "-", NULL},
  { "EMM_CON",        "at+sysinfo",    KNI_SYSINFO_EMM_CONNECTION, 0,  "-", emm_connection},
  { "RRC_CON",        "at+sysinfo",    KNI_SYSINFO_RRC,            0,  "-", rrc_list},
  { "TIN",            "at+sysinfo",    KNI_SYSINFO_TIN,            0,  "-", tin_list},
  { "SIM",            "at+sysinfo",    KNI_SYSINFO_SIM,            0,  "-", sim_list},
  { "NUMBER",         "at+cnum",       3,                          0,  "-", NULL},
  { "QOS_UL",         "at+cgeqreq?",   3,                          0,  "-", NULL},
  { "QOS_DL",         "at+cgeqreq?",   4,                          0,  "-", NULL},
  { "APN",            "at+cgdcont?",   1,                          0,  "-", NULL},
  { "CALL DROP",      "at+sysinfo",    27,                         0,  "-", calldrop_list},
  { "",               "",              0,                          0,  "-", NULL},
};


//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------

/* Comment ********************************************************************
 * Kernel-I 모듈의 정보를 읽어옵니다.
 *
 */
void
get_kmkl200_status(ZEBOS_LTE_MODULE_t *mod)
{
  enum {
    KNI_DUMMY_CMD= 0,
    KNI_ATE0_CMD,
    KNI_SYSINFO_CMD,
    KNI_CGMR_CMD,
    KNI_CNUM_CMD,
    KNI_GSN_CMD
  };
  char *pCmd[] = {"at", "ate0", "at+sysinfo", "at+cgmr", "at+cnum", "at+gsn", NULL};
  enum {
    KNI_DUMMY= 0,
    KNI_SRV_STATUS,
    KNI_SYS_MODE,
    KNI_ROAM_STATUS,
    KNI_SIM_STATUS,
    KNI_VOICE_DOMAIN,
    KNI_SMS_DOMAIN,
    KNI_MCC,
    KNI_MNC,
    KNI_BAND,
    KNI_CHANNEL,
    KNI_RSSI,
    KNI_RSCP,
    KNI_BIT_ERR_RATE,
    KNI_ECIO,
    KNI_RSRP,
    KNI_RSRQ,
    KNI_LAC,
    KNI_LTE_TAC,
    KNI_LTE_RAC,
    KNI_CELL_ID,
    KNI_EMM_STATE,
    KNI_EMM_SUBSTATE,
    KNI_EMM_CONNECTION_STATE,
    KNI_REG_CAUSE,
    KNI_TIN,
    KNI_RRC_ACTIVE,
  };
  int list_max = (sizeof(kni_state_list)/sizeof(LTE_STATE));
  char buf[LTE_BUF_LENGTH+10] = {0};
  char buf_copy[LTE_BUF_LENGTH+10] = {0};
  char buf_temp[LTE_BUF_LENGTH+10] = {0};
  char *pbuf = buf;
  char *pbuf_copy = buf_copy;
  char *pbuf_temp = buf_temp;
  int read_bytes = 0;
  int value = 0;
  int i = 0;

  // dummy read
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], pbuf);

  // dummy read
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial (pCmd[KNI_DUMMY_CMD], pbuf);

  // 모뎀으로 부터의 echo를 제거합니다.
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial (pCmd[KNI_ATE0_CMD], pbuf);

  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_copy, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial (pCmd[KNI_SYSINFO_CMD], pbuf);
  if (read_bytes > 0)
    {
      memcpy(pbuf_copy, pbuf, read_bytes);
      pbuf_copy[LTE_BUF_LENGTH-1] = '\0';
      memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(pbuf_copy, read_bytes, KNI_SRV_STATUS, pbuf_temp);

      mod->state = pal_atoi(pbuf_temp);
      for (i = 0; i < list_max; i++)
        {
          DBG ("@ %d / %d\n", mod->state , kni_state_list[i].state );
          if (kni_state_list[i].state == mod->state)
            {
              snprintf(mod->state_name, sizeof(mod->state_name), "%s", kni_state_list[i].name);
              break;
            }
        }

      // netork type
      memcpy(pbuf_copy, pbuf, read_bytes);
      pbuf_copy[LTE_BUF_LENGTH-1] = '\0';
      memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(pbuf_copy, read_bytes, KNI_SYS_MODE, pbuf_temp);

      value = pal_atoi(pbuf_temp);
      if (value == 5)
          snprintf(mod->type, sizeof(mod->type), "%s", "WCDMA");
      else if (value == 9)
          snprintf(mod->type, sizeof(mod->type), "%s", "LTE");
      else
          snprintf(mod->type, sizeof(mod->type), "%s", "NONE");


      // netork name
      memcpy(pbuf_copy, pbuf, read_bytes);
      pbuf_copy[LTE_BUF_LENGTH-1] = '\0';
      memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(pbuf_copy, read_bytes, KNI_MNC, pbuf_temp);

      value = pal_atoi(pbuf_temp);
      if (value == 5)
          snprintf(mod->name, sizeof(mod->name), "%s", "SKT");
      else if (value == 8)
          snprintf(mod->name, sizeof(mod->name), "%s", "KT");
      else
          snprintf(mod->name, sizeof(mod->name), "%s", "NONE");


      // rssi
      memcpy(pbuf_copy, pbuf, read_bytes);
      pbuf_copy[LTE_BUF_LENGTH-1] = '\0';
      memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(pbuf_copy, read_bytes, KNI_RSSI, pbuf_temp);
      mod->rssi = pal_atoi(pbuf_temp);

      // rsrp
      memcpy(pbuf_copy, pbuf, read_bytes);
      pbuf_copy[LTE_BUF_LENGTH-1] = '\0';
      memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(pbuf_copy, read_bytes, KNI_RSRP, pbuf_temp);
      mod->rsrp = pal_atoi(pbuf_temp);

      // rsrq
      memcpy(pbuf_copy, pbuf, read_bytes);
      pbuf_copy[LTE_BUF_LENGTH-1] = '\0';
      memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(pbuf_copy, read_bytes, KNI_RSRQ, pbuf_temp);
      mod->rsrq = pal_atoi(pbuf_temp);

      // Ant level area
      // | -128 ~ -100 | -100 ~ -94  | -93  ~ -87 |  -86  ~ 80 |  -80  ~    |
      // |      0      |      1      |      2     |       3    |       4    |
      if (mod->rsrp < -100)
        mod->level = 0;
      else if (mod->rsrp < -94)
        mod->level = 25;
      else if (mod->rsrp < -87)
        mod->level = 50;
      else if (mod->rsrp < -80)
        mod->level = 75;
      else if (mod->rsrp >= -80)
        mod->level = 100;
      else
        mod->level = 0;
    }

  // module info
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_copy, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial (pCmd[KNI_CGMR_CMD], pbuf);
  if (read_bytes > 0)
    lte_strtok(pbuf, read_bytes, KNI_CGMR_INDEX, mod->info);

  // number
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_copy, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial (pCmd[KNI_CNUM_CMD], pbuf);
  if (read_bytes > 0)
    {
      // KT의 경우 전화번호 Index가 차이가 있습니다.
      if (strncmp(mod->name, "SKT", 3) ==0)
        lte_strtok(pbuf, read_bytes, KNI_CNUM_INDEX, mod->number);
      else if (strncmp(mod->name, "KT", 2) ==0)
        lte_strtok(pbuf, read_bytes, KNI_CNUM_INDEX + 2, mod->number);
    }

  // imei
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_copy, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial (pCmd[KNI_GSN_CMD], pbuf);
  if (read_bytes > 0)
    lte_strtok(pbuf, read_bytes, KNI_IMEI_INDEX, mod->imei);

  if (strncmp(mod->name, "KT", 2) == 0)
    {
      memset(mod->manufacturer, 0x00, sizeof(mod->manufacturer));
      memcpy(mod->manufacturer, "Kernel-I", strlen("Kernel-I"));
    }
}

/* Comment ********************************************************************
 * 커널아이 모듈을 통해 SMS를 전송합니다.
 *
 *
 */
int
sendmsg_kmkl200(struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg)
{
  int i = 0;
  int length = 0;
  char result[LTE_BUF_LENGTH + 10];
  char command[LTE_BUF_LENGTH + 10];
  char token_buff[32][32];
  char *ptr;
  enum {
    KNI_DUMMY_CMD= 0,
    KNI_CNUM_CMD,
    KNI_SKT_MOREQ_CMD
  };
  char *pCmd[] = {"at", "at+cnum", "at*skt*moreq", NULL};

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], result);

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], result);

  i = 0;
  memset(result, 0x00, sizeof(result));
  memset(token_buff, 0x00, sizeof(token_buff));
  length = lte_xmit_serial(pCmd[KNI_CNUM_CMD], result);
  DBG("%s \r\n", result);
  if (length > 0)
  {
    ptr = strtok (result," ,.:\"");
    while (ptr != NULL)
    {
        snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
        i++;
        DBG ("[%d] %s\n", (i - 1), ptr);
        ptr = strtok (NULL, " ,.:\"");
    }
    // 두번째 buff의 내용이 전화번호입니다.
    snprintf(mod->number, sizeof(mod->number), "%s", token_buff[1]);
  }

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], result);

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], result);

  memset(command, 0x00, LTE_BUF_LENGTH);
  snprintf(command, sizeof(command), "%s = 0,%s,%s,%s", pCmd[KNI_SKT_MOREQ_CMD], pNumber, mod->number, pMsg);

  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_sms(command, result, AT_WAIT);
  usleep(1000);

  if (length > 0)
  {
    for (i = 1; i < length; i++)
    {
      if (result[i - 1] == 'O' && result[i] == 'K')
        return LTE_OK;
    }
  }

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], result);

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * KNI 모듈의 OTA 명령을 수행합니다.
 *
 */
int
set_kmkl200_ota (int type)
{
  enum {
    KNI_DUMMY_CMD= 0,
    KNI_KT_OTA_CMD,
    KNI_SKT_OTA_CMD
  };
  char *pCmd[] = {"at",
                  "AT*OTASTART=#758353266#646#",
                  "AT$$KTFOTAOPEN=*147359*682*",
                  NULL};
  char buf[LTE_BUF_LENGTH+10] = {0};
  char buf_temp[LTE_BUF_LENGTH+10] = {0};
  char *pbuf = buf;
  char *pbuf_temp = buf_temp;
  int read_bytes = 0;

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], pbuf);

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], pbuf);

  if (type == OTA_TYPE_KT)
    {
      // OTA command
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(pCmd[KNI_KT_OTA_CMD], pbuf);
      DBG("%s \r\n", pbuf);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes, "OK") == LTE_ERROR)
            return -1;
        }
    }
  else if (type == OTA_TYPE_SKT)
    {
      // OAT command
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(pCmd[KNI_SKT_OTA_CMD], pbuf);
      DBG("%s \r\n", pbuf);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes, "OK") == LTE_ERROR)
            return -1;
        }
    }

  return 0;
}

/* Comment ********************************************************************
 * KNI 모듈의 APN을 설정하는 함수입니다.
 * 설정하는 APN type 정해지지 않으면,
 * at+sysinfo 명령을 통해 네트워크를 검색 후 default 값으로 설정하도록 합니다.
 *
 */
int
set_kmkl200_apn (int type, char *apn_url)
{
  enum {
    KNI_DUMMY_CMD= 0,
    KNI_APN_KT_CMD,
    KNI_APN_SKT_CMD
  };
  char *pCmd[] = {"at",
                  "AT+CGDCONT=1,\"IP\",\"privatelte.ktfwing.com\",\"0.0.0.0\",0,0",
                  "AT+CGDCONT=1,\"IP\",\"lte-internet.sktelecom.com\",\"0.0.0.0\",0,0",
                  NULL};
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+1] = {0};
  char cmd_buf[LTE_BUF_LENGTH+1] = {0};
  char *pbuf = buf;
  char *pbuf_temp = buf_temp;
  int read_bytes = 0;
  int value = 0;

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], pbuf);

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[KNI_DUMMY_CMD], pbuf);

  // disable echo
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("ate0", pbuf);

  // find network name (KT or SKT)
  if (type == APN_TYPE_DEFAULT)
    {
      memset (pbuf, 0x00, LTE_BUF_LENGTH);
      memset (pbuf_temp, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial ("at+sysinfo", pbuf);
      if (read_bytes > 0)
        {
          // sysinfo 의 8번째 값을 가지고 판단한다.
          lte_strtok(pbuf, read_bytes, 8, pbuf_temp);
          value = pal_atoi(pbuf_temp);

          if (value == 5)
            type = APN_TYPE_SKT;
          else if (value == 8)
            type = APN_TYPE_KT;
          else
            type = APN_TYPE_NONE;
        }
    }

  if (type == APN_TYPE_KT)
    {
      // APN command
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(pCmd[KNI_APN_KT_CMD], pbuf);

      DBG("%s \r\n", pbuf);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes, "OK") == LTE_ERROR)
            return LTE_ERROR;
        }
    }
  else if (type == APN_TYPE_SKT)
    {
      // APN command
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(pCmd[KNI_APN_SKT_CMD], pbuf);

      DBG("%s \r\n", pbuf);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes, "OK") == LTE_ERROR)
            return LTE_ERROR;
        }
    }
  else if (type == APN_TYPE_USER)
    {
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      memset(cmd_buf, 0x00, LTE_BUF_LENGTH);
      snprintf (cmd_buf,
                LTE_BUF_LENGTH,
                "AT+CGDCONT=1,\"IP\",\"%s\",\"0.0.0.0\",0,0",
                apn_url);
      read_bytes = lte_xmit_serial(cmd_buf, pbuf);

      DBG("apn_url : %s \r\n", apn_url);
      DBG("CMD     : %s \r\n", cmd_buf);
      DBG("pbuf    : %s \r\n", pbuf);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes, "OK") == LTE_ERROR)
            return LTE_ERROR;
        }
    }

  return LTE_OK;
}

/* Comment ********************************************************************
 * KNI 모듈을 사용할 때 KT IOT에 대한 정보를 가져와 출력합니다.
 *
 */
void
get_kmkl200_detail_status (struct cli *cli)
{
  char buf[LTE_BUF_LENGTH+10] = {0};
  char *pbuf = buf;
  int read_bytes = 0;
  int value = 0;
  int i = 0;
  int index= 0;

  // dummy read
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", pbuf);

  // dummy read
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", pbuf);

  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("ate0", pbuf);

  while (kni_list[i].at_cmd[0] == 'a' || kni_list[i].at_cmd[0] == 'A')
    {
      memset (pbuf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(kni_list[i].at_cmd, pbuf);
      if (read_bytes > 0)
        {
          // 예외로 처리되는 부분입니다.
          // APN 은 따로 처리가 되어야 합니다.
          if (strncmp (kni_list[i].subject, "APN", strlen("APN")) == 0)
            {
              lte_strtok_by_user (pbuf,
                                 read_bytes,
                                 kni_list[i].index,
                                 kni_list[i].contents,
                                 ":\n\r");
              cli_out (cli, " %s:%s \n",
                             kni_list[i].subject,
                             kni_list[i].contents);

              // i 값이 증가되어야 하는 것에 주의해야 합니다.
              // 무한루프에 빠질 수 있습니다.
              i++;
              continue;
            }

          lte_strtok(pbuf,
                    read_bytes,
                    kni_list[i].index,
                    kni_list[i].contents);

          value = atoi (kni_list[i].contents);
          kni_list[i].result = value;

          // EMM STATE 값에 따라 EMM_SUB의 값이 바뀌여야 합니다.
          if (kni_list[i].index == KNI_SYSINFO_EMM_STATE)
            {
              switch(value)
                {
                  case 1:
                    kni_list[i + 2].list = emm_sub_1;
                    break;
                  case 2:
                    kni_list[i + 2].list = emm_sub_2;
                    break;
                  case 3:
                    kni_list[i + 2].list = emm_sub_3;
                    break;
                  default:
                    break;
                }
            }

          if (kni_list[i].list != NULL)
            {
              index = 0;
              // 각 구조체의 마지막 index를 -1로 하여
              // 리스트의 마지막임을 확인합니다.
              while (kni_list[i].list[index].index != -1)
                {
                  if (value == kni_list[i].list[index].index)
                    {
                      snprintf (kni_list[i].contents,
                                sizeof(kni_list[i].contents),
                                "%s",
                                kni_list[i].list[index].contents);
                    }
                  index++;
                }
            }
        }
      else
        continue;

      cli_out (cli, " %s: %s\n",
                       kni_list[i].subject,
                       kni_list[i].contents);
      i++;
    }
}

/* Comment ********************************************************************
 * KNI 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_kmkl200_apn (lte_apn_info_t *apn_info)
{
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_copy[LTE_BUF_LENGTH+1] = {0};
  char *pbuf = buf;
  char *pbuf_copy = buf_copy;
  int read_bytes = 0;

  // dummy read
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", pbuf);

  // dummy read
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", pbuf);

  // disable echo
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("ate0", pbuf);

  // read access point name
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  memset (pbuf_copy, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at+cgdcont?", pbuf);
  if (read_bytes > 0)
    {
      // apn이 출력되는 라인을 모두 복사합니다.
      // show lte apn detail CLI를 동작시킬 때 표기됩니다.
      lte_strtok_by_user (pbuf,
                         read_bytes,
                         1,
                         apn_info->long_apn,
                         " :\n\r");

      snprintf (pbuf_copy,
                LTE_BUF_LENGTH,
                "%s",
                apn_info->long_apn);

      // apn 만 복사합니다.
      lte_strtok_by_user (pbuf_copy,
                         read_bytes,
                         3,
                         apn_info->short_apn,
                         "\"\n\r");
      return;
    }

  snprintf (apn_info->short_apn, sizeof(apn_info->short_apn), "Unknown");
  snprintf (apn_info->long_apn, sizeof(apn_info->long_apn), "Unknown");
}

/* Comment ********************************************************************
 * KMKL200 의 네트워크 상태를 리턴합니다.
 * IN SERVICE 상태가 아니면 LTE_ERROR로 리턴하도록 합니다.
 *
 */
int
read_kmkl200_network_status (void)
{
  int status = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char network_status_buf[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("ate0", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  // 모듈의 상태값을 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  memset (network_status_buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+sysinfo", buf);
  DBG("%s \r\n", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  lte_strtok (buf, read_bytes, 1, network_status_buf);

  status = atoi (network_status_buf);
 
  //  KMKL200 모듈의 상태값 0x02 가 서비스 가능 상태입니다. 
  if (status == 0x02)
    return LTE_OK;
  else  
    return LTE_ERROR;
}


#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
