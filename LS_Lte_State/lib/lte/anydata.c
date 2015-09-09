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

LTE_STATE anydata_state_list[] = {
  {0x00, "NO SERVICE"},
  {0x01, "LIMIT SERVICE"},
  {0x02, "IN SERVICE"},
  {0x03, "LIMITED REGIONAL SERVICE"},
  {0x04, "POWER SAVE OF DEEP SLEEP"},
};

//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * Anydata 모듈의 정보를 얻어옵니다.
 *
 */
void
get_anydata_status(ZEBOS_LTE_MODULE_t *mod)
{
  int i = 0;
  int j = 0;
  int k = 0;
  int read_bytes = 0;
  char *ptr;
  char buf[LTE_BUF_LENGTH + 1] = {0};
  char lgtver[32];
  char token_buff[32][64];
  enum {
    ANY_STATE = 1,
    ANY_SRV_STATUS = 2,
    ANY_LTE_STATUS = 5,
    ANY_RSSI = 10,
    ANY_RSRP = 12,
    ANY_RSRQ = 13,
  };

  // dummy read
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("ate1", buf);

  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT*LTESTATE?", buf);
  DBG("@ %s \r\n", buf);
  if (read_bytes > 0)
  {
    ptr = strtok (buf," ,.:\"");
    while (ptr != NULL)
    {
        snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
        i++;
        DBG("@ [%d] %s \r\n", (i - 1), ptr);
        ptr = strtok (NULL, " ,.:\"");
    }

    // network type
    if (pal_atoi(token_buff[ANY_STATE]) == 0)
        snprintf(mod->type, sizeof(mod->type), "%s", "GSM");
    else if (pal_atoi(token_buff[ANY_STATE]) == 1)
        snprintf(mod->type, sizeof(mod->type), "%s", "UMTS");
    else if (pal_atoi(token_buff[ANY_STATE]) == 2)
        snprintf(mod->type, sizeof(mod->type), "%s", "LTE");
    else
        snprintf(mod->type, sizeof(mod->type), "%s", "NONE");

    // netork name
    snprintf(mod->name, sizeof(mod->name), "%s", "LGU+");

    // 서비스 상태
    mod->state = pal_atoi(token_buff[ANY_SRV_STATUS]);
    if (mod->state >= 0 && mod->state < ARRAY_SIZE (anydata_state_list))
      pal_snprintf (mod->state_name, MAX_BUF_LENGTH, "%s",
                    anydata_state_list[mod->state].name);
    else
      pal_snprintf (mod->state_name, MAX_BUF_LENGTH, "%s", "UNKNOWN");

    // 신호 세기 읽어오기
    // ANYDATA 모듈의 버전에 따라 rsrp / rsrq는 못 읽어 올 수도 있다. 
    mod->rssi = -1 * pal_atoi(token_buff[ANY_RSSI]);
    mod->rsrp = -1 * pal_atoi(token_buff[ANY_RSRP]);
    mod->rsrq = -1 * pal_atoi(token_buff[ANY_RSRQ]);

    // LG Type1 과 동일하게 rsrp 값으로 처리합니다.
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
  }

  // APN을 읽어옵니다.
  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  read_bytes = lte_xmit_serial("at+cgdcont?", buf);
  DBG("@ %s \r\n", buf);

  if (read_bytes > 0)
    { 

      // APN을 알아내기 위해 at$lgtapnname 의 결과물 중에서 
      // APN을 나타내는 문자열을 찾습니다. 
      // 기본 값으로는 m2m-router 를 사용하도록 합니다.  
      if (lte_lookup_string_from_buf (buf, read_bytes, "mobiletelemetry") == LTE_OK)
        snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "Mobile");
      else if (lte_lookup_string_from_buf (buf, read_bytes, "internet") == LTE_OK)
        snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "Internet");
      else
        snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "M2M-Router");
    }

#if 1
  //////////////////////////////////////////////////////////////
  // Get phone number
  //////////////////////////////////////////////////////////////
  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT$LGTMIN?", buf);
  DBG("@ %s \r\n", buf);

  if (read_bytes > 0)
  {
      ptr = strtok (buf," ,.:\"\n\r");
      while (ptr != NULL)
      {
          snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
          i++;
          DBG("@ [%d] %s \r\n", (i - 1), ptr);
          ptr = strtok (NULL, " ,.:\"\n\r");
      }
      // 3번째 buff의 내용이 전화번호입니다.
      snprintf(mod->number, sizeof(mod->number), "%s", token_buff[2]);
  }
#endif

  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT+CIMI", buf);
  DBG("@ %s \r\n", buf);

  if (read_bytes > 0)
  {
      ptr = strtok (buf," ,.:\"\n\r");
      while (ptr != NULL)
      {
          snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
          i++;
          DBG("@ [%d] %s \r\n", (i - 1), ptr);
          ptr = strtok (NULL, " ,.:\"\n\r");
      }
      // 2번째의 내용이 IMSI 값을 가지고 있습니다.
      snprintf(mod->number2, sizeof(mod->number2), "%s", token_buff[1]);
  }


  //////////////////////////////////////////////////////////////
  // LGT Version
  //////////////////////////////////////////////////////////////
  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(lgtver, 0x00, sizeof(lgtver));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT$LGTVER?", buf);
  DBG("@ %s \r\n", buf);

  if (read_bytes > 0)
  {
      ptr = strtok (buf," ,.:\"\n\r");
      while (ptr != NULL)
      {
          snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
          i++;
          DBG("@ [%d] %s \r\n", (i - 1), ptr);
          ptr = strtok (NULL, " ,.:\"\n\r");
      }
      // 3번째의 buff의 내용이 모듈의 버전을 나타냅니다.
      snprintf(lgtver, sizeof(lgtver), "%s", token_buff[2]);
  }

  //////////////////////////////////////////////////////////////
  // USIM type + LGT Version
  //////////////////////////////////////////////////////////////
  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT+CGDCONT?", buf);
  DBG("@ %s \r\n", buf);

  if (read_bytes > 0)
  {
      ptr = strtok (buf," ,.:\"\n\r");
      while (ptr != NULL)
      {
          snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
          i++;
          DBG("@ [%d] %s \r\n", (i - 1), ptr);
          ptr = strtok (NULL, " ,.:\"\n\r");
      }
      // 4번째의 buff의 내용이 USIM의 타입을 나타냅니다.
      // mobile 또는 internet 으로 표기됩니다.
      snprintf(mod->info, sizeof(mod->info), "%s / %s", token_buff[4], lgtver);
  }

  //////////////////////////////////////////////////////////////
  // imei string (GSN)
  //////////////////////////////////////////////////////////////
  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT+GSN", buf);
  DBG("%s \r\n", buf);

  if (read_bytes > 0)
  {
      ptr = strtok (buf, " ,.:\"\n\r");
      while (ptr != NULL)
      {
          snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
          i++;
          DBG ("[%d] %s\n", (i - 1), ptr);
          ptr = strtok (NULL, " ,.:\"\n\r");
      }
      // 두번째 내용이 imei 정보입니다.
      memcpy(mod->imei, token_buff[1], strlen(token_buff[1]) - 1);
  }

  i = j = k = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT*RUIMID?", buf);
  DBG("%s \r\n", buf);

  if (read_bytes > 0)
  {
      ptr = strtok (buf, " ,.:\"\n\r");
      while (ptr != NULL)
      {
          snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
          i++;
          DBG ("[%d] %s\n", (i - 1), ptr);
          ptr = strtok (NULL, " ,.:\"\n\r");
      }
      // 3번째 내용이 ruimid 정보입니다.
      snprintf(mod->ruimid, sizeof(mod->ruimid), "%s", token_buff[2]);
  }

  return;
}

/* Comment ********************************************************************
 * ANYDATA 모듈의 APN을 설정하는 함수입니다.
 *
 */
int
set_anydata_apn (int type, char *apn_url)
{
  int read_bytes;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char apn_cmd[LTE_BUF_LENGTH+1] = {0};

  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(apn_cmd, 0x00, LTE_BUF_LENGTH);

  if (type == APN_TYPE_DEFAULT)
    snprintf (apn_cmd,
              LTE_BUF_LENGTH,
              "%s",
              "AT+CGDCONT=1,\"IP\",\"mobiletelemetry.lguplus.co.kr\",\"\",0,0,0" );
  else if (type == APN_TYPE_LGU)
    snprintf (apn_cmd,
              LTE_BUF_LENGTH,
              "AT+CGDCONT=1,\"IP\",\"%s\",\"\",0,0,0",
              apn_url);
  else
    return LTE_ERROR;

  // dummy read
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", buf);

  read_bytes = lte_xmit_serial(apn_cmd, buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  DBG("apn_url : %s \r\n", apn_url);
  DBG("CMD     : %s \r\n", apn_cmd);
  DBG("buf     : %s \r\n", buf);

  if (lte_lookup_string_from_buf(buf, read_bytes, "OK") == LTE_OK)
    return LTE_OK;

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * ANYDATA 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_anydata_apn (lte_apn_info_t *apn_info)
{
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_copy[LTE_BUF_LENGTH+1] = {0};
  int read_bytes = 0;

  // dummy read
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("ate1", buf);

  // read access point name
  memset (buf, 0x00, LTE_BUF_LENGTH);
  memset (buf_copy, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at+cgdcont?", buf);
  DBG("@ %s \r\n", buf);

  if (read_bytes <= 0)
    {
      snprintf (apn_info->short_apn, sizeof(apn_info->short_apn), "Unknown");
      snprintf (apn_info->long_apn, sizeof(apn_info->long_apn), "Unknown");
      return;
    }

  // apn이 출력되는 라인을 모두 복사합니다.
  // show lte apn detail CLI를 동작시킬 때 표기됩니다.
  lte_strtok_by_user (buf, read_bytes, 2, apn_info->long_apn, " :\n\r");

  snprintf (buf_copy, LTE_BUF_LENGTH, "%s", apn_info->long_apn);

  // apn 만 복사합니다.
  lte_strtok_by_user (buf_copy, read_bytes, 3, apn_info->short_apn, "\"\n\r");

  return;
}

/* Comment ********************************************************************
 * ANYDATA 모듈의 네트워크 상태를 리턴합니다.
 * IN SERVICE 상태가 아니면 LTE_ERROR로 리턴하도록 합니다.
 *
 */
int
read_anydata_network_status(ZEBOS_LTE_MODULE_t *mod)
{
  int i = 0;
  int read_bytes = 0;
  int status;
  char *ptr;
  char buf[LTE_BUF_LENGTH + 1] = {0};
  char token_buff[32][64];
  enum {
    ANY_STATE = 1,
    ANY_SRV_STATUS = 2,
    ANY_LTE_STATUS = 5,
    ANY_RSSI = 10,
  };

  // dummy read
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("ate1", buf);

  i = 0;
  memset(buf, 0x00, sizeof(buf));
  memset(token_buff, 0x00, sizeof(token_buff));
  read_bytes = lte_xmit_serial("AT*LTESTATE?", buf);
  DBG("@ %s \r\n", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  ptr = strtok (buf," ,.:\"");
  while (ptr != NULL)
  {
      snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
      i++;
      DBG("@ [%d] %s \r\n", (i - 1), ptr);
      ptr = strtok (NULL, " ,.:\"");
  }

  // 서비스 상태
  status = pal_atoi(token_buff[ANY_SRV_STATUS]);

  if (status == 0x02)
    return LTE_OK;
  else
    return LTE_ERROR;
}

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
