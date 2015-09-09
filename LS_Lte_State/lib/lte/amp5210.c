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

// AMT 모듈은 숫자로 표기되지 않고 문자열로 표기된다.
// 아래의 리스트는 이해를 돕기 위해 작성하였다.
LTE_STATE amt_state_list[] = {
  {0x00, "SERVICE NONE"},
  {0x01, "NO SERVICE"},
  {0x02, "LIMITED"},
  {0x03, "IN SERVICE"},
  {0x04, "LIMITED REGIONAL"},
  {0x05, "POWER SAVE"},
  {0x06, "MAX"},
};


//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * AMT 모듈의 정보를 읽어옵니다.
 *
 */
void
get_amp5210_status (ZEBOS_LTE_MODULE_t *mod)
{
  enum {
    AMT_DUMMY_CMD= 0,
    AMT_NSI_CMD,
    AMT_CSQ_CMD,
    AMT_GMM_CMD,
    AMT_GSN_CMD,
    AMT_CNUM_CMD
  };
  char *pCmd[] = {"at",
                    "at@nsi",
                    "at+csq",
                    "at+gmm",
                    "at+gsn",
                    "at+cnum",
                    NULL};
  char buf[LTE_BUF_LENGTH+10] = {0};
  char buf_temp[LTE_BUF_LENGTH+10] = {0};
  char *pbuf = buf;
  char *pbuf_temp = buf_temp;
  int rssi = 0;
  int read_bytes = 0;

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_DUMMY_CMD], pbuf);

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_DUMMY_CMD], pbuf);

  //state
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_NSI_CMD], pbuf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      lte_strtok(pbuf, read_bytes, AMT_STATE_INDEX, mod->state_name);

      if (!pal_strncmp(mod->state_name, "IN", 2))
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "IN SERVICE");
    }

  // network name / network type
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_NSI_CMD], pbuf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      // network name
      {
        if (lte_lookup_string_from_buf(pbuf, read_bytes, "skt") == LTE_OK ||
            lte_lookup_string_from_buf(pbuf, read_bytes, "SKT") == LTE_OK )
          snprintf(mod->name, sizeof(mod->name), "%s", "SKT");
        else if (lte_lookup_string_from_buf(pbuf, read_bytes, "olleh") == LTE_OK||
            lte_lookup_string_from_buf(pbuf, read_bytes, "kt") == LTE_OK ||
            lte_lookup_string_from_buf(pbuf, read_bytes, "OLLEH") == LTE_OK ||
            lte_lookup_string_from_buf(pbuf, read_bytes, "KT") == LTE_OK)
          snprintf(mod->name, sizeof(mod->name), "%s", "KT");
        else
          snprintf(mod->name, sizeof(mod->name), "%s", "UNKNOWN");
      }

      // network type
      lte_strtok(pbuf, read_bytes, AMT_NETTYPE_INDEX, mod->type);
  }

  // RSSI (signal quality)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_CSQ_CMD], pbuf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      lte_strtok(pbuf, read_bytes, AMT_RSSI_INDEX, pbuf_temp);

      rssi = pal_atoi(pbuf_temp);
      //DBG("@buff = %s (%d)\r\n", buff, rssi );
      if (rssi == 99)
        rssi = 0;

      mod->level = (rssi / 6) * 25; // 100% 단위로 표시하기 위하여 25를 곱합니다.
      if (mod->level > 100)
        mod->level = 100;
      else if (mod->level < 0)
        mod->level = 0;

      // 실제 dBm 값 구하는 공식((113 - (mod->rssi * 2)) * -1) )
      mod->rssi = ((113 - (rssi * 2)) * -1);
    }

  // module info (GMM)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_GMM_CMD], pbuf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    lte_strtok(pbuf, read_bytes, AMT_GMM_INDEX, mod->info);

  // IMEI (GSN)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_GSN_CMD], pbuf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    lte_strtok(pbuf, read_bytes, AMT_IMEI_INDEX, mod->imei);

  // NUMBER (CNUM)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_CNUM_CMD], pbuf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      // KT의 경우 전화번호 Index가 차이가 있습니다.
      if (strncmp(mod->name, "SKT", 3) ==0)
        lte_strtok(pbuf, read_bytes, AMT_CNUM_INDEX, mod->number);
      else if (strncmp(mod->name, "KT", 2) ==0)
        lte_strtok(pbuf, read_bytes, AMT_CNUM_INDEX + 2, mod->number);
    }
}


/* Comment ********************************************************************
 * AMT 모듈을 통해 SMS를 전송합니다.
 *
 */
int
sendmsg_amp5210(struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg)
{
  int i = 0;
  int length = 0;
  char result[LTE_BUF_LENGTH + 10];
  char command[LTE_BUF_LENGTH + 10];
  char end_char = 0x1a;  // ctrl + z
  enum {
    MEC_DUMMY_CMD= 0,
    MEC_CMGF_CMD,
    MEC_CMGS_CMD
  };
  char *pCmd[] = {"at", "at+cmgf=1", "at+cmgs", NULL};

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_DUMMY_CMD], result);

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_DUMMY_CMD], result);

  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_CMGF_CMD], result);
  DBG("%s \r\n", result);
  if (length > 0)
  {
    snprintf(command, sizeof(command), "%s=\"%s\"", pCmd[MEC_CMGS_CMD], pNumber);

    DBG(" @ %s \r\n", command);

    memset(result, 0x00, LTE_BUF_LENGTH);
    length = lte_xmit_sms(command, result, AT_NO_WAIT);
    usleep(1000);

    memset(result, 0x00, LTE_BUF_LENGTH);
    length = lte_xmit_sms(pMsg, result, AT_NO_WAIT);
    usleep(1000);

    memset(result, 0x00, LTE_BUF_LENGTH);
    length = lte_xmit_sms(&end_char, result, AT_WAIT);
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
    length = lte_xmit_serial(pCmd[MEC_DUMMY_CMD], result);
  }

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * AMT 모듈의 OTA 명령을 수행합니다.
 *
 */
int
set_amp5210_ota (int type)
{
  enum {
    AMT_DUMMY_CMD= 0,
    AMT_KT_OTA_CMD,
    AMT_SKT_OTA_CMD,
    AMT_QMIAUTO_CMD
  };
  char *pCmd[] = {"at",
                  "at@open=#758353266#646#",
                  "AT@OPEN=*147359*682*",
                  "at@qmiauto=1",
                  NULL};
  char buf[LTE_BUF_LENGTH+10] = {0};
  char buf_temp[LTE_BUF_LENGTH+10] = {0};
  char *pbuf = buf;
  char *pbuf_temp = buf_temp;
  int read_bytes = 0;

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_DUMMY_CMD], pbuf);

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_DUMMY_CMD], pbuf);

  if (type == OTA_TYPE_KT)
    {
      // OTA command
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(pCmd[AMT_KT_OTA_CMD], pbuf);
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
      read_bytes = lte_xmit_serial(pCmd[AMT_SKT_OTA_CMD], pbuf);
      DBG("%s \r\n", pbuf);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes, "OK") == LTE_ERROR)
            return -1;
        }
    }

  // QMIAUTO command
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_QMIAUTO_CMD], pbuf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      if (lte_lookup_string_from_buf(pbuf, read_bytes, "OK") == LTE_ERROR)
        return -1;
    }

  return 0;
}

/* Comment ********************************************************************
 * AMT 모듈의 APN을 설정하는 함수입니다.
 * 설정하는 APN type 정해지지 않으면,
 *
 */
int
set_amp5210_apn (int type, char *apn_url)
{
  enum {
    AMT_DUMMY_CMD= 0,
    AMT_APN_KT_CMD,
    AMT_APN_SKT_CMD
  };
  char *pCmd[] = {"at",
                  "AT+CGDCONT=1,\"IP\",\"lte.ktfwing.com\",\"0.0.0.0\",0,0",
                  "AT+CGDCONT=1,\"IP\",\"lte-internet.sktelecom.com\",\"0.0.0.0\",0,0",
                  NULL};
  char buf[LTE_BUF_LENGTH+1] = {0};
  char cmd_buf[LTE_BUF_LENGTH+1] = {0};
  char *pbuf = buf;
  int read_bytes = 0;

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_DUMMY_CMD], pbuf);

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_DUMMY_CMD], pbuf);

  // spc lock 해제
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("AT@spc=267277", pbuf);

  // find network name (KT or SKT)
  if (type == APN_TYPE_DEFAULT)
    {
      // network name / network type
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial("at@nsi", pbuf);
      DBG("%s \r\n", pbuf);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes, "skt") == LTE_OK ||
              lte_lookup_string_from_buf(pbuf, read_bytes, "SKT") == LTE_OK )
            type = APN_TYPE_SKT;
          else if (lte_lookup_string_from_buf(pbuf, read_bytes, "olleh") == LTE_OK||
                   lte_lookup_string_from_buf(pbuf, read_bytes, "kt") == LTE_OK ||
                   lte_lookup_string_from_buf(pbuf, read_bytes, "OLLEH") == LTE_OK ||
                   lte_lookup_string_from_buf(pbuf, read_bytes, "KT") == LTE_OK)
            type = APN_TYPE_KT;
          else
            type = APN_TYPE_NONE;
      }
    }

  if (type == APN_TYPE_KT)
    {
      // APN command
      memset(pbuf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(pCmd[AMT_APN_KT_CMD], pbuf);

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
      read_bytes = lte_xmit_serial(pCmd[AMT_APN_SKT_CMD], pbuf);

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
      DBG("read_byte : %d \r\n", read_bytes);
      if (read_bytes > 0)
        {
          if (lte_lookup_string_from_buf(pbuf, read_bytes + 2, "OK") == LTE_ERROR)
            return LTE_ERROR;
        }
    }

  return LTE_OK;
}

/* Comment ********************************************************************
 * AMT 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_amp5210_apn (lte_apn_info_t *apn_info)
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
                         2,
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


#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
