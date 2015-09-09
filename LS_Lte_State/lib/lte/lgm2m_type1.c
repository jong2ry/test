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
#include "if_lte.h"
#include "lib/lte/serial.h"
#include "lib/lte/lgm2m_type1.h"
#include "common/common_drv.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof (A) / sizeof ((A)[0]))
#endif /* !ARRAY_SIZE */

//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
//#define DBG    console_print
#define DBG(x...)

//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment *******************************************************
 * get LG M2M module information
 * LG M2M 모듈은 ltestate 라는 별도의 실행파일을 실행시켜
 * 상태값을 읽어옵니다.
 *
 */
void
get_lgm2m_type1_status(ZEBOS_LTE_MODULE_t *mod)
{
  char buf[LTE_BUF_LENGTH+10]={0};
  char *pbuf = buf;
  int read_bytes = 0;

  /* get lgu+ network type */
    {
    if (0 == access(LTEIF_LGU_MOBILE, F_OK) )
      snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "Mobile");
    else if (0 == access(LTEIF_LGU_INTERNET, F_OK) )
      snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "Internet");
    else
      snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "M2M-Router");
    }

  /* netork name        */
  snprintf (mod->name, sizeof(mod->name), "%s", "LGU+");

  /* lte network type   */
  /* LGU+ use only LTE  */
  snprintf (mod->type, sizeof(mod->type), "%s", "LTE");

  {
    int shm_id = 0;
    void *pShm = NULL;
    LTE_CM_STATE_SHM_t lte_state;

    memset (&lte_state, 0x00, sizeof(LTE_CM_STATE_SHM_t));

    shm_id = shmget ((key_t)LTE_CM_STATE_KEY,
                      sizeof(LTE_CM_STATE_SHM_t),
                      IPC_CREAT|SHM_RDONLY);

    if (shm_id < 0)
      return;

    if ((void *)-1 == (pShm = shmat(shm_id, (void *)0, 0)))
      return;

    memcpy((char *)&lte_state, (char *)pShm, sizeof(LTE_CM_STATE_SHM_t));
    shmdt(pShm);

    mod->state = lte_state.CmState;
    // m2mpcm의 STATUS 값은 m2mpcm으로 부터 문자열 형태로 받아옵니다.
    snprintf(mod->state_name, sizeof(mod->state_name), "%s", lte_state.CmStateStr);
    // UICC 값은 m2mpcm으로 부터 문자열 형태로 받아옵니다.
    snprintf(mod->lgu_uicc, sizeof(mod->lgu_uicc), "%s", lte_state.UiccStateStr);

    if (strlen(lte_state.IMSI) >= 15)
      {
        // IMSI값중의 앞이 7자리를 제외한 값이 전화번호가 됩니다.
        // 010 . 012 번호를 구분할 수 없기 때문에 01X라는 번호를 사용합니다.
        snprintf(mod->number, sizeof(mod->number), "01X%s", &lte_state.IMSI[7]);
        snprintf(mod->number2, sizeof(mod->number2), "%s", lte_state.IMSI);
      }
  }

  //imei
  memset (pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+cgsn", pbuf);
  ltedebug ("%s \r\n", pbuf);
  if (read_bytes > 0)
    lte_strtok (pbuf, read_bytes, 1, mod->imei);

  // RF와 관련된 데이터를 읽어옵니다.
  {
    int shm_rf_id = 0;
    void *pShm_rf = NULL;
    LTE_RF_DEBUG_SHM_t lte_rf;

    memset (&lte_rf, 0x00, sizeof(LTE_RF_DEBUG_SHM_t));

    shm_rf_id = shmget ((key_t)LTE_CM_RF_DEBUG_KEY,
                      sizeof(LTE_RF_DEBUG_SHM_t),
                      IPC_CREAT|SHM_RDONLY);

    if (shm_rf_id < 0)
      return;

    if ((void *)-1 == (pShm_rf = shmat(shm_rf_id, (void *)0, 0)))
      return;

    memcpy((char *)&lte_rf, (char *)pShm_rf, sizeof(LTE_RF_DEBUG_SHM_t));
    shmdt(pShm_rf);

    mod->rssi = lte_rf.rssi;
    mod->rsrp = lte_rf.rsrp;
    mod->rsrq = lte_rf.rsrq;
    mod->sinr = lte_rf.sinr;

    /*
      1) 안테나 5 개 표시 RSRP >=-85dBm
      2) 안테나 4 개 표시 -95 dBm <= RSRP < -85 dBm
      3) 안테나 3 개 표시 -105 dBm <= RSRP < -95 dBm
      4) 안테나 2 개 표시 -115 dBm <= RSRP < -105 dBm
      5) 안테나 1 개 표시 RSRP < -115 dBm
      6) 안테나만 포시된 상태
      7) No Service 상태

      RSRP의 표준 규격은 -140 dBm ~ -44 dBm 입니다.
      그러므로 -140 dBm 이하의 값은 0으로 표기하도록 합니다.
    */
    if (mod->rsrp >= 0)
      mod->level = 0;
    else if (mod->rsrp < -140)
      mod->level = 0;
    else if (mod->rsrp >= -140 && mod->rsrp < -115)
      mod->level = 20;
    else if (mod->rsrp >= -115 && mod->rsrp < -105)
      mod->level = 40;
    else if (mod->rsrp >= -105 && mod->rsrp < -95)
      mod->level = 60;
    else if (mod->rsrp >= -95 && mod->rsrp < -85)
      mod->level = 80;
    else if (mod->rsrp >= -85)
      mod->level = 100;
    else
      mod->level = 0;

    // m2mpcm의 상태값이 "CONNECTED"가 아니면 level 은 0으로 셋팅합니다.
    if (mod->state != 16)
      mod->level = 0;

    // 사용하지는 않지만 작업상태를 남기기 위하여 주석처리합니다.
    // RSRP 값이 -80 dbm이상이고 (AND 조건) SINR 값이 0 이상일때
    // 신호 상태가 좋은 것으로 표기합니다.
    //if (mod->rsrp >= -80 && mod->sinr >= 0)
    //  snprintf(mod->lgu_sig_state, sizeof(mod->lgu_sig_state), "%s", "GOOD SIGNAL");
    //else
    //  snprintf(mod->lgu_sig_state, sizeof(mod->lgu_sig_state), "%s", "BAD SIGGNAL");
  }

  return;
}


/* Comment ********************************************************************
 * LGM2M TYPE1 모듈의 APN을 설정하는 함수입니다.
 *
 */
int
set_lgm2m_type1_apn (int type, char *apn_url)
{
  char syscmd[LTE_BUF_LENGTH+1] = {0};

  memset (syscmd, 0x00, sizeof(syscmd));
  if (type == APN_TYPE_DEFAULT)
    {
      snprintf (syscmd, sizeof(syscmd), "touch %s", LTEIF_LGU_M2M);
      system (syscmd);

      unlink (LTEIF_LGU_MOBILE);
      unlink (LTEIF_LGU_INTERNET);

      system ("cp -rf  /usr/share/m2mp-cm/LteCmCfg_m2m.xml  /usr/share/m2mp-cm/LteCmCfg.xml");
      system ("sync");
      return LTE_OK;
    }

  if (strncmp (apn_url, "mobiletelemetry", strlen("mobiletelemetry")) == 0)
    {
      snprintf (syscmd, sizeof(syscmd), "touch %s", LTEIF_LGU_MOBILE);
      system (syscmd);

      unlink (LTEIF_LGU_INTERNET);
      unlink (LTEIF_LGU_M2M);

      system ("cp -rf  /usr/share/m2mp-cm/LteCmCfg_mobile.xml  /usr/share/m2mp-cm/LteCmCfg.xml");
      system ("sync");
    }
  else if (strncmp (apn_url, "internet", strlen("internet")) == 0)
    {
      snprintf (syscmd, sizeof(syscmd), "touch %s", LTEIF_LGU_INTERNET);
      system (syscmd);

      unlink (LTEIF_LGU_MOBILE);
      unlink (LTEIF_LGU_M2M);

      system ("cp -rf  /usr/share/m2mp-cm/LteCmCfg_internet.xml  /usr/share/m2mp-cm/LteCmCfg.xml");
      system ("sync");
    }
  else if (strncmp (apn_url, "m2m-router", strlen("m2m-router")) == 0)
    {
      snprintf (syscmd, sizeof(syscmd), "touch %s", LTEIF_LGU_M2M);
      system (syscmd);

      unlink (LTEIF_LGU_MOBILE);
      unlink (LTEIF_LGU_INTERNET);

      system ("cp -rf  /usr/share/m2mp-cm/LteCmCfg_m2m.xml  /usr/share/m2mp-cm/LteCmCfg.xml");
      system ("sync");
    }
  else
    return LTE_ERROR;

  return LTE_OK;
}


/* Comment ********************************************************************
 * LGM2M TYPE1 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_lgm2m_type1_apn (lte_apn_info_t *apn_info)
{
  /* get lgu+ network type */
  if (0 == access(LTEIF_LGU_MOBILE, F_OK) )
    {
      snprintf (apn_info->short_apn,
                sizeof(apn_info->short_apn),
                "mobiletelemetry.lguplus.co.kr");
      snprintf (apn_info->long_apn,
                sizeof(apn_info->long_apn),
                "Found /media/disk0/.lgu_mobile");
    }
  else if (0 == access(LTEIF_LGU_INTERNET, F_OK) )
    {
      snprintf (apn_info->short_apn,
                sizeof(apn_info->short_apn),
                "internet.lguplus.co.kr");
      snprintf (apn_info->long_apn,
                sizeof(apn_info->long_apn),
                "Found /media/disk0/.lgu_internet");
    }
  else if (0 == access(LTEIF_LGU_M2M, F_OK) )
    {
      snprintf (apn_info->short_apn,
                sizeof(apn_info->short_apn),
                "m2m-router.lguplus.co.kr");
      snprintf (apn_info->long_apn,
                sizeof(apn_info->long_apn),
                "Found /media/disk0/.lgu_m2m");
    }
  else
    {
      snprintf (apn_info->short_apn,
                sizeof(apn_info->short_apn),
                "m2m-router.lguplus.co.kr");
      snprintf (apn_info->long_apn,
                sizeof(apn_info->long_apn),
                "Default apn");
    }

  return;
}


/* Comment ********************************************************************
 * LTER의 연결상태를 리턴합니다.
 * 오직 _CM_STATE_CONNECTED 인 상태일 때만 LTE_OK를 리턴합니다.
 *
 */
int
read_lgm2m_type1_network_status (void)
{
  int shm_id = 0;
  void *pShm = NULL;
  LTE_CM_STATE_SHM_t lte_state;

  memset (&lte_state, 0x00, sizeof(LTE_CM_STATE_SHM_t));

  shm_id = shmget ((key_t)LTE_CM_STATE_KEY,
                    sizeof(LTE_CM_STATE_SHM_t),
                    IPC_CREAT|SHM_RDONLY);

  if (shm_id < 0)
    return LTE_ERROR;

  if ((void *)-1 == (pShm = shmat(shm_id, (void *)0, 0)))
    return LTE_ERROR;

  memcpy((char *)&lte_state, (char *)pShm, sizeof(LTE_CM_STATE_SHM_t));
  shmdt(pShm);

  if (lte_state.CmState == _CM_STATE_CONNECTED)
    return LTE_OK;
  else
    return LTE_ERROR;
}


/* Comment *******************************************************
 * 현재 동작중이 M2MPCM의 상태를 리턴합니다.
 *
 */
int
is_lgm2m_type1_m2mpcm(void)
{
  FILE    *fp = NULL;
  int     read_size = 0;
  char    buf[LTE_BUF_LENGTH];
  int     ret = 0;

  fp = popen(M2M_GET_M2MPCM_CMD, "r");

  if (!fp)
    return LTE_ERROR;

  memset(buf, 0x00, sizeof(buf));
  read_size = fread( (void*)buf, sizeof(char), LTE_BUF_LENGTH - 1, fp );

  pclose(fp);

  ret = atoi(buf);

  if (ret == 1)
    return LTE_OK;
  else
    return LTE_ERROR;
}

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
