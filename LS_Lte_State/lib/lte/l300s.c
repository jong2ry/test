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
#include "lib/lte/l300s.h"
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

LTE_STATE l300s_state_list[] = {
  {0x00, "NO SERVICE"},
  {0x01, "IN SERVICE"},
  {0x02, "LIMIT SERVICE"},
};


//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * 팬택의 PM-L300S의 상태 정보를 읽어옵니다.
 *
 */
void
get_l300s_status(ZEBOS_LTE_MODULE_t *mod)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  //state
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*regsts?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    {
      memset (buf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok (buf, read_bytes, 2, buf_temp);

      mod->state = atoi (buf_temp);
      if (mod->state >= 0 && mod->state < ARRAY_SIZE (l300s_state_list))
        pal_snprintf (mod->state_name, MAX_BUF_LENGTH, "%s",
                      l300s_state_list[mod->state].name);
      else
        pal_snprintf (mod->state_name, MAX_BUF_LENGTH, "%s", "UNKNOWN");
    }

  // imei
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*imei?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 2, mod->imei);

  // number
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*minfo", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 3, mod->number);

  // info (모듈 이름)
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*minfo", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 2, mod->info);

  // name (항상 SKT로 보여집니다)
  snprintf (mod->name, MAX_BUF_LENGTH, "%s", "SKT");

  // network 망 확인
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at*antlvl?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 3, mod->type);

  // rssi
  // 안테나 값은 0 - 10 까지 주어집니다.
  // 하지만 rssi 값은 제공 되지 않았습니다.
  // 그래서 임의로 rssi 값을 출력하도록 합니다.
  // NTmore LM200Q 모듈의 data를 근거로 작성하였습니다.
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(buf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at*antlvl?", buf);
  DBG("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      lte_strtok (buf, read_bytes, 2, buf_temp);
      mod->level = atoi(buf_temp) * 10;
    }
}

/* Comment ********************************************************************
 * PM-L300S 모듈을 통해 SMS를 전송합니다.
 *
 */
int
sendmsg_l300s(struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg)
{
  int i = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH + 1] = {0};;
  char command[LTE_BUF_LENGTH + 1] = {0};;
  char msg_buf[LTE_BUF_LENGTH + 1] = {0};

  // SMS 초기화
  // 아래 3가지의 초기화 과정을 거쳐야 SMS 가 전송됩니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("AT+CPMS=\"ME\",\"ME\",\"ME\"", buf);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("AT+CMGF=1", buf);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("AT+CNMI=2,1,0,0", buf);

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // 전화번호 얻어오기
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("AT*MINFO", buf);
  DBG("%s \r\n", buf);

  // 모듈의 리턴 메시지를 체크합니다.
  if (read_bytes <= 0)
    return LTE_ERROR;

  // 전화번호를 저장합니다.
  lte_strtok(buf, read_bytes, 3, mod->number);

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // 8bit unicode msg로 변환.
  // hex 값은 대문자로 표기되어야 합니다.
  read_bytes = strlen(pMsg);
  memset(msg_buf, 0x00, LTE_BUF_LENGTH);
  for (i = 0; i < read_bytes; i++)
    sprintf (&msg_buf[i*2], "%2X", pMsg[i]);

  // 8bit type으로 영어 단문만 보내도록 합니다.
  memset(command, 0x00, LTE_BUF_LENGTH);
  snprintf (command,
            sizeof(command),
            "%s=%s,%s,1,1,%s",
            "AT*SMSMO",
            pNumber,
            mod->number,
            msg_buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_sms(command, buf, AT_WAIT);
  usleep(1000);

  // 모듈의 리턴 메시지를 체크합니다.
  if (read_bytes <= 0)
    return LTE_ERROR;

  // SMS 전송 상태를 체크합니다.
  if (lte_lookup_string_from_buf (buf, read_bytes, "OK") == LTE_OK)
    return LTE_OK;


  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * PM-L300S 의 OTA 명령을 수행합니다.
 *
 */
int
set_l300s_ota (int type)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // OTA command
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT*OTASTART", buf);
  DBG ("%s \r\n", buf);

  // 에러 상태입니다.
  if (read_bytes <= 0)
    return LTE_ERROR;

  // OTA 진행 시 성공적으로 개통하면 LTE_OK 로 리턴됩니다.
  if (lte_lookup_string_from_buf(buf, read_bytes, "OK") == LTE_OK)
    return LTE_OK;

  // 이미 개통된 상태이면 DONE 로 리턴됩니다.
  if (lte_lookup_string_from_buf(buf, read_bytes, "OTADONE") == LTE_OK)
    return LTE_OK;

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * PM-L300S 모듈의 APN을 설정하는 함수입니다.
 * SKT로만 설정하도록 합니다.
 *
 */
int
set_l300s_apn (int type, char *apn_url)
{
  int temp = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+1] = {0};
  char user_apn[LTE_BUF_LENGTH+1] = {0};
  char apn_cmd[] = "AT+CGDCONT=1,\"IP\",\"lte-internet.sktelecom.com\",\"0.0.0.0\",0,0";

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // SKT 관련 APN만 처리됩니다.
  if (type == APN_TYPE_DEFAULT || type == APN_TYPE_SKT)
    {
      // APN Change ensable
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      temp = lte_xmit_serial("AT*SKT*QOSON=1", buf_temp);

      // APN command
      memset(buf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial(apn_cmd, buf);
      DBG ("%s \r\n", buf);

      if (read_bytes <= 0)
        return LTE_ERROR;

      // APN Change disable
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      temp = lte_xmit_serial("AT*SKT*QOSON=0", buf_temp);

      if (lte_lookup_string_from_buf(buf, read_bytes, "OK") == LTE_OK)
        return LTE_OK;
    }
  else if (type == APN_TYPE_USER)
    {
      // APN Change ensable
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      temp = lte_xmit_serial("AT*SKT*QOSON=1", buf_temp);

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

      // APN Change disable
      memset(buf_temp, 0x00, LTE_BUF_LENGTH);
      temp = lte_xmit_serial("AT*SKT*QOSON=0", buf_temp);

      if (read_bytes <= 0)
        return LTE_ERROR;

      if (lte_lookup_string_from_buf(buf, read_bytes, "OK") == LTE_OK)
        return LTE_ERROR;
    }

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * PM-L300S 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_l300s_apn (lte_apn_info_t *apn_info)
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
 * L300S 의 네트워크 상태를 리턴합니다.
 * IN SERVICE 상태가 아니면 LTE_ERROR로 리턴하도록 합니다.
 *
 */
int 
read_l300s_network_status (void)
{
  int status = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char network_status[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;
 
  // 모듈의 상태값을 읽어옵니다. 
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*regsts?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    {
      memset (network_status, 0x00, LTE_BUF_LENGTH);
      lte_strtok (buf, read_bytes, 2, network_status);

      status = atoi (network_status);

      // L300S의 상태값 0x01 이 서비스 가능 상태입니다.
      if (status == 0x01)
        return LTE_OK;
      else
        return LTE_ERROR;
    }

  return LTE_ERROR;
}


#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
