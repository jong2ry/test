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

LTE_STATE ntlm200q_state_list[] = {
  {0x00, "NO SERVICE"},
  {0x01, "LIMIT SERVICE"},
  {0x02, "IN SERVICE"},
  {0x03, "LIMITED REGIONAL SERVICE"},
  {0x04, "POWER SAVE OF DEEP SLEEP"},
};

LTE_STATE ntlm200q_sysmode_list[] = {
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

//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
//#define DBG    console_print
#define DBG(x...)

//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
void
get_ntlm200q_status(ZEBOS_LTE_MODULE_t *mod)
{
  int index = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // 모듈의 상태값을 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*st*regsts?", buf);
  DBG("%s \r\n", buf);
  if (read_bytes > 0)
    {
      memset (buf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok (buf, read_bytes, 2, buf_temp);

      mod->state = atoi (buf_temp);
      if (mod->state >= 0 && mod->state < ARRAY_SIZE (ntlm200q_state_list))
        pal_snprintf (mod->state_name, MAX_BUF_LENGTH, "%s",
                      ntlm200q_state_list[mod->state].name);
      else
        pal_snprintf (mod->state_name, MAX_BUF_LENGTH, "%s", "UNKNOWN");
    }

  // 모듈의 IMEI 값을 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*st*imei?", buf);
  DBG ("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok(buf, read_bytes, 2, mod->imei);

  // 모듈의 전화번호를 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*st*msisdn?", buf);
  DBG ("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok(buf, read_bytes, 2, mod->number);

  // 모듈의 네트워크 망 상태를 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*st*sysmode?", buf);
  DBG ("%s \r\n", buf);
  if (read_bytes > 0)
    {
      memset (buf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok (buf, read_bytes, 2, buf_temp);

      index = atoi (buf_temp);
      if (index >= 0 && index < ARRAY_SIZE (ntlm200q_sysmode_list))
        pal_snprintf (mod->type, MAX_BUF_LENGTH, "%s",
                      ntlm200q_sysmode_list[index].name);
      else
        pal_snprintf (mod->type, MAX_BUF_LENGTH, "%s", "UNKNOWN");
    }

  // 네크워크 사업자 표시
  snprintf (mod->name, MAX_BUF_LENGTH, "%s", "SKT");

  // 모듈의 버전명을 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*st*info?", buf);
  DBG ("%s \r\n", buf);
  if (read_bytes > 0)
    lte_strtok (buf, read_bytes, 2, mod->info);

  // 신호값을 읽어옵니다.
  // NTLM-200Q 모듈은 안테나값으로 표현되어 집니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*st*antenna?", buf);
  DBG ("%s \r\n", buf);
  if (read_bytes > 0)
    {
      memset (buf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok (buf, read_bytes, 2, buf_temp);

      mod->level = atoi (buf_temp) * 25;
    }
}

/* Comment ********************************************************************
 * NTLM200Q 모듈을 통해 SMS를 전송합니다.
 *
 */
int
sendmsg_ntlm200q(struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg)
{
  int i = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH + 1] = {0};;
  char command[LTE_BUF_LENGTH + 1] = {0};;
  char msg_buf[LTE_BUF_LENGTH + 1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);
  if (read_bytes <= 0)
    return LTE_ERROR;

  // 전화번호 얻어오기
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("AT*SKT*DIAL", buf);
  DBG ("%s \r\n", buf);
  
  // 모듈의 리턴 메시지를 체크합니다.
  if (read_bytes <= 0)
    return LTE_ERROR;
 
  // 전화번호를 저장합니다.  
  lte_strtok(buf, read_bytes, 2, mod->number);

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", buf);

  // 모듈의 리턴 메시지를 체크합니다.
  if (read_bytes <= 0)
    return LTE_ERROR;

  // 8bit unicode msg로 변환.
  // hex 값은 대문자로 표기되어야 합니다.
  read_bytes = strlen(pMsg);
  memset(msg_buf, 0x00, LTE_BUF_LENGTH);
  for (i = 0; i < read_bytes; i++)
    {
      sprintf (&msg_buf[i*2],
               "%2X",
               pMsg[i]);
    }

  // 8bit type으로 영어 단문만 보내도록 합니다.
  memset(command, 0x00, LTE_BUF_LENGTH);
  snprintf (command,
            sizeof(command),
            "%s=1,%s,%s,%s",
            "AT*SMS*EXMO",
            pNumber,
            mod->number,
            msg_buf);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_sms(command, buf, AT_WAIT);
  usleep(1000);

  // 모듈의 리턴 메시지를 체크합니다.
  if (read_bytes <= 0)
    return LTE_ERROR;

  // SMS 전송 상태를 체크합니다.
  if (lte_lookup_string_from_buf (buf, read_bytes, "OK") == LTE_OK)
    return LTE_OK;

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial("at", buf);

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * NTmore NTLM200Q 모듈 (NTLM200Q) 의 OTA 명령을 수행합니다.
 *
 */
int
set_ntlm200q_ota (int type)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH + 1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // OTA command
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("AT*SET*OTASTART", buf);
  DBG ("%s \r\n", buf);

  // 에러 상태입니다.
  if (read_bytes <= 0)
    return LTE_ERROR;

  // OTA 진행 시 성공적으로 개통하면 LTE_OK 로 리턴됩니다.
  if (lte_lookup_string_from_buf (buf, read_bytes, "OK") == LTE_OK)
    return LTE_OK;

  // 이미 개통된 상태이면 DONE 로 리턴됩니다.
  if (lte_lookup_string_from_buf (buf, read_bytes, "DONE") == LTE_OK)
    return LTE_OK;

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * NTLM200Q 모듈의 APN을 설정하는 함수입니다.
 * SKT로만 설정하도록 합니다.
 *
 */
int
set_ntlm200q_apn (int type, char *apn_url)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH + 1] = {0};
  char user_apn[LTE_BUF_LENGTH + 1] = {0};
  char apn_cmd[] = "at*net*profile=1,\"IP\",\"lte-internet.sktelecom.com\",\"0.0.0.0\",0,0";

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // SKT 관련 APN만 처리됩니다.
  if (type == APN_TYPE_DEFAULT || type == APN_TYPE_SKT)
    {
      memset (buf, 0x00, LTE_BUF_LENGTH);
      read_bytes = lte_xmit_serial (apn_cmd, buf);
      DBG ("%s \r\n", buf);

      if (read_bytes <= 0)
        return LTE_ERROR;

      if (lte_lookup_string_from_buf (buf, read_bytes, "OK") == LTE_OK)
        return LTE_OK;
    }
  else if (type == APN_TYPE_USER)
    {
      memset (buf, 0x00, LTE_BUF_LENGTH);
      memset (user_apn, 0x00, LTE_BUF_LENGTH);
      snprintf (user_apn,
                LTE_BUF_LENGTH,
                "at*net*profile=1,\"IP\",\"%s\",\"0.0.0.0\",0,0",
                apn_url);
      read_bytes = lte_xmit_serial (user_apn, buf);
      DBG ("apn_url : %s \r\n", apn_url);
      DBG ("CMD     : %s \r\n", user_apn);
      DBG ("buf     : %s \r\n", buf);

      if (read_bytes <= 0)
        return LTE_ERROR;

      if (lte_lookup_string_from_buf (buf, read_bytes, "OK") == LTE_OK)
        return LTE_OK;
    }

  return LTE_ERROR;
}

/* Comment ********************************************************************
 * NTmore NTLM-200Q (NTLM200Q)  모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_ntlm200q_apn (lte_apn_info_t *apn_info)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char apn_profile[LTE_BUF_LENGTH+1] = {0};
  char apn_addr[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  memset (buf, 0x00, LTE_BUF_LENGTH);
  memset (apn_addr, 0x00, LTE_BUF_LENGTH);
  memset (apn_profile, 0x00, LTE_BUF_LENGTH);

  // APN 정보를 읽어옵니다.
  read_bytes = lte_xmit_serial ("at*net*profile?", apn_profile);

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
 * NTLM-200Q 모듈의 자세한 정보를 출력합니다.
 * 특별히 정해진 포멧 없이 모듈에서 나오는 값을 그대로 출력합니다.
 *
 */
void
get_ntlm200q_detail_status(struct cli *cli)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  // APN 정보를 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*skt*dbs", buf);

  if (read_bytes <= 0)
    {
      cli_out (cli, " Unknown\n");
      return;
    }
  
  cli_out (cli, "%s\n", buf);
}


/* Comment ********************************************************************
 * NTLM-200Q 의 네트워크 상태를 리턴합니다.
 * IN SERVICE 상태가 아니면 LTE_ERROR로 리턴하도록 합니다.
 *
 */
int
read_ntlm200q_network_status (void)
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
  read_bytes = lte_xmit_serial ("at*st*regsts?", buf);
  DBG("%s \r\n", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  lte_strtok (buf, read_bytes, 2, network_status);

  status = atoi (network_status);
 
  // NTLM-200Q의 상태값 0x02 가 서비스 가능 상태입니다. 
  if (status == 0x02)
    return LTE_OK;
  else  
    return LTE_ERROR;
}


/* Comment ********************************************************************
 * NTLM-200Q 의 RMNET 을 동작시킵니다.
 *
 */
int
enable_ntlm200q_rmnet (void)
{
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};

  // dummy at 명령을 수행해 모듈을 초기화합니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  // 모듈의 상태값을 읽어옵니다.
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial ("at*data*call=1", buf);
  DBG("%s \r\n", buf);

  if (read_bytes <= 0)
    return LTE_ERROR;

  if (lte_lookup_string_from_buf (buf, read_bytes, "CME LTE_ERROR") == LTE_OK)
    return LTE_ERROR;
  else
    return LTE_OK;
}




#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
