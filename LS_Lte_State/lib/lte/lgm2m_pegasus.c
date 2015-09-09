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
#include "lib/lte/lgm2m_pegasus.h"
#include "common/common_drv.h"
#include "lib/lte/serial.h" 


//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof (A) / sizeof ((A)[0]))
#endif /* !ARRAY_SIZE */

//#define DBG    console_print
#define DBG(x...)


//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * get LG M2M Typ2 module information
 * LG M2M type2 모듈은 소켓 통신을 통해서 상태값을 읽어옵니다.
 *
 */
void
get_lgm2m_pegasus_status(ZEBOS_LTE_MODULE_t *mod)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char real_msg[LTE_BUF_LENGTH+1] = {0};


  // 아래 3가지 항목은 고정된 값으로 사용됩니다. 
  snprintf(mod->name, sizeof(mod->name), "%s", "LGU+");
  snprintf(mod->type, sizeof(mod->type), "%s", "LTE");

  //////////////////////////////////////////////////////////////
  // DUMMY READ
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT", buf);

  //////////////////////////////////////////////////////////////
  // GET INFO
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT$LGTSTINFO?", buf);

  DBG("%s \r\n", buf);

  if (read_bytes > 0)
    {
      lte_strtok (buf, read_bytes, 2, real_msg);
      DBG("#real_msg : %s \r\n", real_msg);
      switch (real_msg[0])
        {
          case 'I':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "IN SERVICE");
            break;

          case 'A':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "Access State");
            break;

          case 'P':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "Paging State");
            break;

          case 'T':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "IN SERVICE");
            break;

          case 'N':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "No Service");
            break;

          default:
             snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "Unknown");
            break;
        }
    }

  //////////////////////////////////////////////////////////////
  // GET IMEI
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT+CGSN", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 1, mod->imei);

  //////////////////////////////////////////////////////////////
  // GET MDN
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT$LGTMDN?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 2, mod->number);

  //////////////////////////////////////////////////////////////
  // GET APN
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT$LGTAPNNAME?", buf);
  DBG("%s \r\n", buf);
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

  //////////////////////////////////////////////////////////////
  // GET RSSI
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT$LGTRSSI?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    {
      lte_strtok (buf, read_bytes, 3, real_msg);
      mod->rsrp = pal_atoi(real_msg);
    }

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

  return;
}

/* Comment ********************************************************************
 * LG M2M PEGASUS 모듈의 APN을 설정하는 함수입니다.
 *
 */
int
set_lgm2m_pegasus_apn (int type, char *apn_url)
{
  char buf[LTE_BUF_LENGTH+1] = {0};
  char apn_cmd[LTE_BUF_LENGTH+1] = {0};
  int read_bytes;

  memset (apn_cmd, 0x00, LTE_BUF_LENGTH);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  if (type == APN_TYPE_DEFAULT)
    snprintf (apn_cmd,
              LTE_BUF_LENGTH,
              "%s",
              "AT$LGTAPNNAME= 0,m2m-router.lguplus.co.kr");
  else if (type == APN_TYPE_LGU)
    snprintf (apn_cmd,
              LTE_BUF_LENGTH,
              "AT$LGTAPNNAME= 0,%s",
              apn_url);
  else
    return LTE_ERROR;

  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial (apn_cmd, buf);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);
 
  return LTE_OK;
}

/* Comment ********************************************************************
 * LG M2M PEGASUS 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_lgm2m_pegasus_apn (lte_apn_info_t *apn_info)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at$lgtapnname?", buf);

  if (read_bytes <= 0)
    {
      snprintf (apn_info->short_apn, sizeof(apn_info->short_apn), "Unknown");
      snprintf (apn_info->long_apn, sizeof(apn_info->long_apn), "Unknown");
      return ;
    }

  // apn이 출력되는 라인을 모두 복사합니다.
  // show lte apn detail CLI를 동작시킬 때 표기됩니다.
  lte_strtok_by_user (buf, read_bytes, 2, apn_info->long_apn, "\n\r");
  snprintf (buf, LTE_BUF_LENGTH, "%s", apn_info->long_apn);

  // apn 만 복사합니다.
  lte_strtok_by_user (buf, read_bytes, 1, apn_info->short_apn, ",\"\n\r");
}


/* Comment ********************************************************************
 * LG M2M PEGASUS 모듈의 상태를 리턴합니다.
 *
 */
int
read_lgm2m_pegasus_network_status (void)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char real_msg[LTE_BUF_LENGTH+1] = {0};

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT", buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT$LGTSTINFO?", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;
  
  lte_strtok (buf, read_bytes, 2, real_msg);

  if (real_msg[0] == 'I' || real_msg[0] == 'T')
    return LTE_OK;
  else
    return LTE_ERROR;
}


#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
