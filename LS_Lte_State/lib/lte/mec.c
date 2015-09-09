#include "pal.h"

#ifdef HAVE_LTE_INTERFACE
#include <unistd.h>
#include <fcntl.h>
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
#include "lib/lte/mec.h"
#include "lib/lte/serial.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof (A) / sizeof ((A)[0]))
#endif /* !ARRAY_SIZE */


/* Comment *******************************************************
 * MobileECO 모듈의 정보를 읽어옵니다.
 *
 */
void
get_mec_status(ZEBOS_LTE_MODULE_t *mod)
{
  int i = 0;
  int j = 0;
  int k = 0;
  int offset = 0;
  int length = 0;
  char result[LTE_BUF_LENGTH + 10] = {0};
  char buff[LTE_BUF_LENGTH + 10] = {0};
  char *ptr;
  char token_buff[32][32];
  enum {
    MEC_DUMMY_CMD= 0,
    MEC_SYSINFO_CMD,
    MEC_COPS_CMD,
    MEC_GMM_CMD,
    MEC_CNUM_CMD,
    MEC_GSN_CMD
  };
  char *pCmd[] = {"at", "at&sysinfo", "at+cops?", "at+gmm", "at+cnum", "at+gsn", NULL};

  // dummy read
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_DUMMY_CMD], result);

  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_DUMMY_CMD], result);

  // state
  i = j = k = 0;
  memset(result, 0x00, LTE_BUF_LENGTH);
  memset(buff, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_SYSINFO_CMD], result);
  ltedebug("%s \r\n", result);
  if (length > 0)
    {
    ptr = strtok (result,",.:\r\n\"");
    while (ptr != NULL)
    {
        snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
        i++;
        ltedebug ("state [%d] %s\n", (i - 1), ptr);
        ptr = strtok (NULL, ",.:\r\n\"");
    }

    // 4번째 내용이 상태를 표시하는 값입니다.
    snprintf(mod->state_name, sizeof(mod->state_name), "%s", token_buff[3]);

    if (!pal_strncmp(mod->state_name, "SRV", 3))
        snprintf(mod->state_name, sizeof(mod->state_name), "%s", "IN SERVICE");
  }

  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_SYSINFO_CMD], result);
  ltedebug("%s \r\n", result);
  if (length > 0)
        {
    // network type
            {
      if (lte_lookup_string_from_buf(result, length, "LTE") == LTE_OK)
        snprintf(mod->type, sizeof(mod->type), "%s", "LTE");
      else if (lte_lookup_string_from_buf(result, length, "WCDMA") == LTE_OK)
        snprintf(mod->type, sizeof(mod->type), "%s", "WCDMA");
      else
        snprintf(mod->type, sizeof(mod->type), "%s", "NONE");
    }
            }

  // RSSI (signal quality)
  i = j = k = 0;
  memset(buff, 0x00, LTE_BUF_LENGTH);
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_SYSINFO_CMD], result);
  ltedebug("%s \r\n", result);
  if (length > 0)
  {
    // 체크를 위하여 5byte ('RSSI:')를 비교합니다.
    for (i = 0; i < length - 5; i++)
    {
      if (result[i] == 'R' &&
          result[i+1] == 'S' &&
          result[i+2] == 'S' &&
          result[i+3] == 'I' &&
          result[i+4] == ':')
          {
        offset = i + 5;
        // 신호세기는 5byte를 넘지 않습니다.
        for (j = offset; j < offset + 5; j++)
            {
          // RSSI 값만을 가지고 오도록 합니다.
          if (result[offset] == ' ' ||  result[offset] == 0x0d)
              break;

          ltedebug("[%d] %c\r\n", j, result[j]);
          buff[k++] = result[j];
            }
          }
      else
        continue;
        }
    mod->rssi = pal_atoi(buff);
    }

  // RSRP (signal quality)
  i = j = k = 0;
  memset(buff, 0x00, LTE_BUF_LENGTH);
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_SYSINFO_CMD], result);
  ltedebug("%s \r\n", result);
  if (length > 0)
  {
    // 체크를 위하여 5byte ('RSRP:')를 비교합니다.
    for (i = 0; i < length - 5; i++)
    {
      if (result[i] == 'R' &&
          result[i+1] == 'S' &&
          result[i+2] == 'R' &&
          result[i+3] == 'P' &&
          result[i+4] == ':')
      {
        offset = i + 5;
        // 신호세기는 5byte를 넘지 않습니다.
        for (j = offset; j < offset + 5; j++)
        {
          // RSSI 값만을 가지고 오도록 합니다.
          if (result[offset] == ' ' ||  result[offset] == 0x0d)
            break;

          ltedebug("[%d] %c\r\n", j, result[j]);
          buff[k++] = result[j];
        }
      }
      else
        continue;
    }
    mod->rsrp = pal_atoi(buff);

    // Ant level area
    // | -128  ~ -120 | -120 ~ -113 | -113 ~ -106 |  -106 ~ 0  |
    // |      0       |      1      |      2      |      3     |
    if (mod->rsrp <= -120)
      mod->level = 0;
    else if (mod->rsrp > -120 && mod->rsrp <= -113)
      mod->level = 33;
    else if (mod->rsrp > -113 && mod->rsrp <= -106)
      mod->level = 66;
    else if (mod->rsrp > -106 && mod->rsrp < 0)
      mod->level = 100;
    else
      mod->level = 0;
  }

  // RSRQ (signal quality)
  i = j = k = 0;
  memset(buff, 0x00, LTE_BUF_LENGTH);
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_SYSINFO_CMD], result);
  ltedebug("%s \r\n", result);
  if (length > 0)
  {
    // 체크를 위하여 5byte ('RSRQ:')를 비교합니다.
    for (i = 0; i < length - 5; i++)
    {
      if (result[i] == 'R' &&
          result[i+1] == 'S' &&
          result[i+2] == 'R' &&
          result[i+3] == 'Q' &&
          result[i+4] == ':')
      {
        offset = i + 5;
        // 신호세기는 5byte를 넘지 않습니다.
        for (j = offset; j < offset + 5; j++)
        {
          // RSSI 값만을 가지고 오도록 합니다.
          if (result[offset] == ' ' ||  result[offset] == 0x0d)
            break;

          ltedebug("[%d] %c\r\n", j, result[j]);
          buff[k++] = result[j];
        }
      }
      else
        continue;
    }
    mod->rsrq = pal_atoi(buff);
  }

  // module name (COPS)
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_COPS_CMD], result);
  //ltedebug("%s \r\n", result);
  if (length > 0)
  {
    if (lte_lookup_string_from_buf(result, length, "skt") == LTE_OK ||
        lte_lookup_string_from_buf(result, length, "SKT") == LTE_OK )
      snprintf(mod->name, sizeof(mod->name), "%s", "SKT");
    else if (lte_lookup_string_from_buf(result, length, "olleh") == LTE_OK ||
        lte_lookup_string_from_buf(result, length, "kt") == LTE_OK ||
        lte_lookup_string_from_buf(result, length, "OLLEH") == LTE_OK||
        lte_lookup_string_from_buf(result, length, "KT") == LTE_OK )
      snprintf(mod->name, sizeof(mod->name), "%s", "KT");
    else
      snprintf(mod->name, sizeof(mod->name), "%s", "UNKNOWN");
  }
  else
    snprintf(mod->name, sizeof(mod->name), "%s", "UNKNOWN");

  // module info (GMM)
  i = j = k = 0;
  memset(buff, 0x00, LTE_BUF_LENGTH);
  memset(result, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_GMM_CMD], result);
  //ltedebug("%s \r\n", result);
  if (length > 0)
  {
    // 모델명은 대문자로 시작하게 됩니다.
    // 문자열에서 대문자로 시작하는 부분만 찾아서
    // 처음부분을 offset 값으로 설정합니다.
    for (offset = 0; offset < length; offset++)
    {
      if (result[offset] >= 'A' && result[offset] <= 'Z')
        break;
    }

    // offset 부터 줄바꿈이 있는 구간이 모델명이 됩니다.
    for (i = offset; i < length; i++)
    {
      if (result[i] == 0x0d)
        break;

      //ltedebug("@ %d - [%c] \r\n", result[i]);
      buff[k++] = result[i];
    }
    //ltedebug("@buff = %s \r\n", buff);
    memcpy(mod->info, buff, strlen(buff));
  }

  // number (CNUM)
  i = j = k = 0;
  memset(result, 0x00, sizeof(result));
  memset(token_buff, 0x00, sizeof(token_buff));
  length = lte_xmit_serial(pCmd[MEC_CNUM_CMD], result);
  ltedebug("%s \r\n", result);
  if (length > 0)
  {
    ptr = strtok (result," ,.:\"");
    while (ptr != NULL)
    {
        snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
        i++;
        ltedebug ("[%d] %s\n", (i - 1), ptr);
        ptr = strtok (NULL, " ,.:\"");
    }
    // 두번째 buff의 내용이 전화번호입니다.
    snprintf(mod->number, sizeof(mod->number), "%s", token_buff[1]);
  }

  // imei string (GSN)
  i = j = k = 0;
  memset(result, 0x00, LTE_BUF_LENGTH);
  memset(buff, 0x00, LTE_BUF_LENGTH);
  length = lte_xmit_serial(pCmd[MEC_GSN_CMD], result);
  ltedebug("%s \r\n", result);
  if (length > 0)
  {
    ptr = strtok (result," ,.:\n");
    while (ptr != NULL)
    {
        snprintf(token_buff[i], sizeof(token_buff[i]), "%s", ptr);
        i++;
        ltedebug ("[%d] %s\n", (i - 1), ptr);
        ptr = strtok (NULL, " ,.:\n");
    }
    // 두번째 내용이 imei 정보입니다.
    memcpy(mod->imei, token_buff[1], strlen(token_buff[1]) - 1);
  }

}


/* Comment *******************************************************
 * MobileECO 모듈을 통해 SMS를 전송합니다.
 *
 */
int
sendmsg_mec(struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg)
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
  ltedebug("%s \r\n", result);
  if (length > 0)
  {
    snprintf(command, sizeof(command), "%s=\"%s\"", pCmd[MEC_CMGS_CMD], pNumber);

    ltedebug(" @ %s \r\n", command);

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




#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
