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
#include "lib/lte/tm400.h"
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

LTE_STATE tm400_state_list[] = {
  {0x00, "NO SERVICE"},
  {0x01, "IN SERVICE"},
  {0x02, "LIMIT SERVICE"},
};


//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * TM400 의 상태 정보를 읽어옵니다.
 *
 */
void
get_tm400_status(ZEBOS_LTE_MODULE_t *mod)
{
  int read_bytes = 0;
  int value = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+1] = {0};
  char value_temp[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  //state
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+creg?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    {
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(buf, read_bytes, 3, buf_temp);

      value = atoi(buf_temp);
      mod->state = value;

      if (value == 0)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "DISCONNECTED");
      else if (value == 1)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "IN SERVCIE");
      else if (value == 2)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "SEARCHING");
      else if (value == 3)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "LIMITED SERVICE");
      else if (value == 4)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "IN SERVICE (ROAM)");
      else
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "DISCONNECTED");
    }

  // imei
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+gsn", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 1, mod->imei);

  // number
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+cnum", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 2, mod->number);

  // info (모듈 이름)
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+gmm", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 1, mod->info);

  // name (항상 KT로 보여집니다)
  snprintf (mod->name, MAX_BUF_LENGTH, "%s", "KT");

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at$$dscreen", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    {
      // network 망 확인
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      memcpy(buf_temp, buf, read_bytes);
      lte_strtok (buf_temp, read_bytes, 1, mod->type);

      // rsrp
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      memset(value_temp, 0x00, LTE_BUF_LENGTH);
      memcpy(buf_temp, buf, read_bytes);
      lte_strtok (buf_temp, read_bytes, 14, value_temp);
      mod->rsrp = atoi(value_temp);

      // rssi
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      memset(value_temp, 0x00, LTE_BUF_LENGTH);
      memcpy(buf_temp, buf, read_bytes);
      lte_strtok (buf_temp, read_bytes, 20, value_temp);
      mod->rssi = atoi(value_temp);

      // rsrq
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      memset(value_temp, 0x00, LTE_BUF_LENGTH);
      memcpy(buf_temp, buf, read_bytes);
      lte_strtok (buf_temp, read_bytes, 23, value_temp);
      mod->rsrq = atoi(value_temp);
    }

  // 신호세기
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+csq", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    {
      memset(value_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok (buf, read_bytes, 2, value_temp);
      
      // 수신감도 는 0 - 31 까지로 나온다. 
      // 그러므로 6을 나누어서 5단계로 표기하도록 하고,
      // 20을 곱해서 %단위로 변환한다.
      mod->level = (atoi(value_temp) / 6) * 20;

      // 서비스 가능 상태가 아니면 신호세기는 0으로 표기합니다.
      if (mod->state != 0x01)
        mod->level = 0;
    }
}

/* Comment ********************************************************************
 * TM400 모듈을 통해 SMS를 전송합니다.
 * 해당 코드는 사용하지 않도록 합니다.
 *
 */
int
sendmsg_tm400(struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg)
{
  return LTE_ERROR;
}

/* Comment ********************************************************************
 * TM400 의 OTA 명령을 수행합니다.
 *
 */
int
set_tm400_ota (int type)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // OTA command
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT$$OPEN=*147359*682*", buf);
  DBG ("%s \r\n", buf);

  // 에러 상태입니다.
  if (read_bytes <= 0)
    return LTE_ERROR;

  // OTA 진행시 다음과 같은 메시지가 출력됩니다. 
  // 그러므로 바로 ERROR 로 리턴되지 않으면 정상으로 처리하도록 합니다.
  /*
  AT$$OPEN=*147359*682*
  $$TELL:19,번호를 등록 중입니다. 휴대폰 기능이 f한됩?? 잠시만 기??주시기 바랍니다.

  ATTACH WCDMA

  $$TELL:19,번호를 등록 중입니다. 휴대폰 기능이 f한됩?? 잠시만 기??주시기 바랍니다.

  $$TELL:45,메시지 발신 요청 완료

  $$TELL:23,?드에 해당 d보를 등록 중입니다. 완료 후 리샵???
  OK
  at
  OK
  */

  // OTA 진행 시 ERROR 메시가 나오면 ERROR로 표기합니다.
  // 그 외에는 모두 정상으로 출력합니다.
  if (lte_lookup_string_from_buf(buf, read_bytes, "ERROR") == LTE_OK)
    return LTE_ERROR;

  return LTE_OK;
}

/* Comment ********************************************************************
 * TM400 모듈의 APN을 설정하는 함수입니다.
 * SKT로만 설정하도록 합니다.
 *
 */
int
set_tm400_apn (int type, char *apn_url)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char user_apn[LTE_BUF_LENGTH+1] = {0};
  char apn_cmd[] = "AT+CGDCONT=1,\"IP\",\"privatelte.ktfwing.com\",\"0.0.0.0\",0,0";

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // KT 관련 APN만 처리됩니다.
  if (type == APN_TYPE_DEFAULT || type == APN_TYPE_KT)
    {
      // APN command
      memset(buf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(apn_cmd, buf);
      DBG ("%s \r\n", buf);

      if (read_bytes <= 0)
        return LTE_ERROR;

      if (lte_lookup_string_from_buf(buf, read_bytes, "OK") == LTE_OK)
        return LTE_OK;
    }
  else if (type == APN_TYPE_USER)
    {
      // APN command
      memset(buf, 0x00, LTE_BUF_LENGTH);
      memset(user_apn, 0x00, LTE_BUF_LENGTH);
      snprintf (user_apn,
                LTE_BUF_LENGTH,
                "AT+CGDCONT=1,\"IP\",\"%s\",\"0.0.0.0\",0,0",
                apn_url);
      read_bytes = lte_xmit_serial(user_apn, buf);
      DBG ("apn_url : %s \r\n", apn_url);
      DBG ("CMD     : %s \r\n", user_apn);
      DBG ("buf    : %s \r\n",  buf);

      if (read_bytes <= 0)
        return LTE_ERROR;

      if (lte_lookup_string_from_buf(buf, read_bytes, "OK") == LTE_OK)
        return LTE_OK;
    }

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * TM400 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_tm400_apn (lte_apn_info_t *apn_info)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char apn_profile[LTE_BUF_LENGTH+1] = {0};
  char apn_addr[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", buf);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  memset (apn_addr, 0x00, LTE_BUF_LENGTH);
  memset (apn_profile, 0x00, LTE_BUF_LENGTH);
  
  // APN 정보를 읽어옵니다.
  read_bytes = lte_xmit_serial("AT+CGDCONT?", apn_profile);

  // 에러 상태입니다.
  // APN 정보를 표시하지 않습니다.
  if (read_bytes <= 0)
    {
      snprintf (apn_info->short_apn, sizeof(apn_info->short_apn),"Unknown");
      snprintf (apn_info->long_apn, sizeof(apn_info->long_apn), "Unknown");
      return;
    }

  // apn이 출력되는 라인을 모두 복사합니다.
  // show lte apn detail CLI를 동작시킬 때 표기됩니다.
  lte_strtok_by_user (apn_profile,
                     read_bytes,
                     2,
                     apn_info->long_apn,
                     " :\n\r");

  snprintf (apn_addr,
            LTE_BUF_LENGTH,
            "%s",
            apn_info->long_apn);

  // apn 만 복사합니다.
  lte_strtok_by_user (apn_addr,
                     read_bytes,
                     2,
                     apn_info->short_apn,
                     ",\"\n\r");
}



/* Comment ********************************************************************
 * TM400 의 네트워크 상태를 리턴합니다.
 * IN SERVICE 상태가 아니면 LTE_ERROR로 리턴하도록 합니다.
 *
 */
int 
read_tm400_network_status (void)
{
  int status = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char network_status[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // 모듈의 상태값을 읽어옵니다. 
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at+creg?", buf);
  DBG("%s \r\n", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  memset (network_status, 0x00, LTE_BUF_LENGTH);
  lte_strtok(buf, read_bytes, 3, network_status);

  status = atoi(network_status);

  // TM400의 상태값 0x01 이 서비스 가능 상태입니다.
  if (status == 0x01)
    return LTE_OK;
  else
    return LTE_ERROR;
}

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
