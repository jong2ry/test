#include "pal.h"

#ifdef HAVE_LTE_INTERFACE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "lib.h"
#include "cli.h"
#include "lib/buffer.h"
#include "lib/gpio_ctrl.h"
#include "lib/daemon_ctl.h"
#include "common/common_drv.h"

#include "lib/if_lte.h"
#include "lib/lte/serial.h"

#include "lib/lte/l300s.h"
#include "lib/lte/tm400.h"
#include "lib/lte/anydata.h"
#include "lib/lte/amp5210.h"
#include "lib/lte/kmkl200.h"
#include "lib/lte/ntmore.h"
#include "lib/lte/ntlm200q.h"
#include "lib/lte/mec.h"
#include "lib/lte/lgm2m_type1.h"
#include "lib/lte/lgm2m_type2.h"
#include "lib/lte/lgm2m_pegasus.h"
#ifdef HAVE_LOCKOUT
#include <sqlite3.h>
#define USER_DB_PATH DISK0_PATH"/.user.ndb"
#define USER_DB_CREATE_TABLE_STR \
 "CREATE TABLE IF NOT EXISTS account_t" \
"(" \
"username TEXT PRIMARY KEY," \
"failcount INTEGER NOT NULL DEFAULT 0," \
"last_login_time INTEGER NOT NULL DEFAULT 0," \
"last_failed_time INTEGER NOT NULL DEFAULT 0" \
");"
#endif /* HAVE_LOCKOUT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof (A) / sizeof ((A)[0]))
#endif /* !ARRAY_SIZE */

//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
//#define ltedebug    console_print
#define ltedebug(x...)


//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * /dev/sysLED 를 제어합니다.
 *
 */
void
lte_set_sysLED (int cmd)
{
  int fd;

  fd = open("/dev/sysLED", O_RDWR);

  switch (cmd)
    {
      case CMD_SIG_LED_PUP:
        ioctl(fd, SYSLED_IOCTL_SIG_PUP);
        break;

      case CMD_SIG_LED_PDN:
        ioctl(fd, SYSLED_IOCTL_SIG_PDN);
        break;

      case CMD_LTE_LED_PUP:
        ioctl(fd, SYSLED_IOCTL_LTE_PUP);
        break;

      case CMD_LTE_LED_PDN:
        ioctl(fd, SYSLED_IOCTL_LTE_PDN);
        break;

      default:
        break;
    }

  close(fd);
  return;
}


/* Comment ********************************************************************
 * /dev/sysLTE 를 제어합니다.
 *
 */
void
lte_set_sysLTE (int cmd)
{
  int fd;

  fd = open("/dev/sysLTE", O_RDWR);

  switch (cmd)
    {
      case CMD_SYSLTE_PUP:
        ioctl(fd, SYSLTE_IOCTL_PUP);
        break;

      case CMD_SYSLTE_PDN:
        ioctl(fd, SYSLTE_IOCTL_PDN);
        break;

      case CMD_SYSLTE_RESET:
        ioctl(fd, SYSLTE_IOCTL_RESET);
        break;

      default:
        break;
    }

  close(fd);
  return;
}


/* Comment ********************************************************************
 * /dev/sysM2M 의 DI 상태값 / FAC 상태값을 읽어옵니다.
 *
 */
static int
_lte_read_sysM2M (int cmd)
{
  int fd;
  char data = 0;

  fd = open("/dev/sysM2M", O_RDWR);

  switch (cmd)
    {
      case CMD_SYSM2M_DI_READ:
        ioctl(fd, SYSM2M_IOCTL_DI_READ, &data);
        break;

      case CMD_SYSM2M_FAC_READ:
        ioctl(fd, SYSM2M_IOCTL_FAC_READ, &data);
        break;

      default:
        break;
    }

  close(fd);
  return data;
}


/* Comment *******************************************************
 *  Reset lte module.
 */

void
lte_reset (void)
{
  lte_set_sysLTE (CMD_SYSLTE_RESET);
}


/* Comment *******************************************************
 * LTE 제조사를 리턴합니다.
 *
 */
int
lte_get_product(void)
{
#ifdef HAVE_OCTEON
  // VF403은 PPP 를 사용하기 때문에 다른 포트를 사용합니다.
  if (is_nexg_model(NEXG_MODEL_VF403))
    {
      if (0 == access(TTY_KNI1, F_OK) )
        return PRODUCT_KNI;
      else if (0 == access(TTY_ANY, F_OK) )
        return PRODUCT_ANYDATA;
    }
#endif

  if (0 == access(TTY_AMT, F_OK) )
    return PRODUCT_AMT;
  else if (0 == access(TTY_KNI0, F_OK) )
    return PRODUCT_KNI;
  else if (0 == access(TTY_MEC, F_OK) )
    return PRODUCT_MEC;
  else if (0 == access(TTY_ANY, F_OK) )
    return PRODUCT_ANYDATA;
  else if (0 == access(TTY_LGM2M_TYPE1, F_OK) )
    return PRODUCT_LGM2M_TYPE1;
  else if (0 == access(TTY_NT, F_OK) )
    return PRODUCT_NT;
  else if (0 == access(TTY_NTLM200Q, F_OK) )
    return PRODUCT_NTLM200Q;
  else if (0 == access(TTY_L300S, F_OK) )
    return PRODUCT_L300S;
  else if (0 == access(TTY_TM400, F_OK) )
    return PRODUCT_TM400;
  else if (0 == access(TTY_PEGASUS, F_OK) )
    return PRODUCT_PEGASUS;
  else
  {
    // LG TYPE2 모듈의 경우에는 RNDIS 로 동작이 됩니다.
    // 하지만 시리얼 포트를 사용하지 않기 때문에, 운영중에 확인할 방법이 없습니다.
    // 그래서 find 와 grep을 사용하여 /sys/device 를 검색해서 사용했습니다.
    // 하지만 이렇게 사용하면 시스템에 부하를 주기 때문에 사용하지 않도록 합니다.
    // 그래서 해당하는 내용을 주석처리 합니다.
    // 추후에 다시 정리하여 작업하기로 합니다.
    // LG 이노텍 M2M 모듈을 확인합니다.
    //if (is_m2m_type2_vendor() == LTE_OK)
    //  return PRODUCT_LGM2M_TYPE2;

    return PRODUCT_NONE;
  }
}


/* Comment *******************************************************
 * 모듈의 정보를 출력합니다.
 *
 */
void
lte_show_status(struct cli *cli, ZEBOS_LTE_MODULE_t *mod)
{
  int product = PRODUCT_NONE;

  // LTE 모듈의 연결상태를 확인합니다.
  product = lte_get_product();
  if (PRODUCT_NONE == product)
      return;

  cli_out(cli, "\n");
  cli_out(cli, "-------- %s LTE module Information -------- \n", mod->manufacturer );
  cli_out(cli, "\n");

  switch(product)
  {
    case PRODUCT_AMT:
        cli_out (cli, "STATUS  : %s, %s, %s \n", mod->state_name, mod->name, mod->type);
        cli_out (cli, "MODULE  : %s / %s \n", mod->manufacturer, mod->info);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        // mod->type이 LTE인 경우에만 LTE LEVEL을 표시합니다.
        if (pal_strncmp(mod->type, "LTE", 3) == 0)
            cli_out (cli, "LEVEL   : %d% (rssi:%d) \n", mod->level, mod->rssi);
        break;

    case PRODUCT_KNI:
        cli_out (cli, "STATUS  : %s (%d), %s, %s \n", mod->state_name, mod->state, mod->name, mod->type);
        cli_out (cli, "MODULE  : %s / %s \n", mod->manufacturer, mod->info);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        // mod->type이 LTE인 경우에만 LTE LEVEL을 표시합니다.
        if (pal_strncmp(mod->type, "LTE", 3) == 0)
            cli_out (cli, "LEVEL   : %d% (rssi:%d / rsrp:%d / rsrq:%d) \n", mod->level, mod->rssi, mod->rsrp, mod->rsrq);
        break;

    case PRODUCT_MEC:
        cli_out (cli, "STATUS  : %s, %s, %s \n", mod->state_name, mod->name, mod->type);
        cli_out (cli, "MODULE  : %s / %s \n", mod->manufacturer, mod->info);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        // mod->type이 LTE인 경우에만 LTE LEVEL을 표시합니다.
        if(pal_strncmp(mod->type, "LTE", 3) == 0)
            cli_out (cli, "LEVEL   : %d% (rssi:%d / rsrp:%d / rsrq:%d) \n", mod->level, mod->rssi, mod->rsrp, mod->rsrq);
        break;

    case PRODUCT_NT:
        cli_out (cli, "STATUS  : %s, %s, %s \n", mod->state_name, mod->name, mod->type);
        cli_out (cli, "MODULE  : %s / %s \n", mod->manufacturer, mod->info);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        // mod->type이 LTE인 경우에만 LTE LEVEL을 표시합니다.
        if (pal_strncmp(mod->type, "LTE", 3) == 0)
            cli_out (cli, "LEVEL   : %d% (rssi:%d) \n", mod->level, mod->rssi);
        break;

    case PRODUCT_NTLM200Q:
        cli_out (cli, "STATUS  : %s (%d), %s, %s \n", mod->state_name, mod->state, mod->name, mod->type);
        cli_out (cli, "MODULE  : %s / %s \n", mod->manufacturer, mod->info);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        cli_out (cli, "LEVEL   : %d% \n", mod->level);
        cli_out (cli, "VERSION : 4.6-150101-3\n");
        break;

    case PRODUCT_L300S:
        cli_out (cli, "STATUS  : %s (%d), %s, %s \n", mod->state_name, mod->state, mod->name, mod->type);
        cli_out (cli, "MODULE  : %s / %s \n", mod->manufacturer, mod->info);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        cli_out (cli, "LEVEL   : %d% \n", mod->level);
        break;

    case PRODUCT_TM400:
        cli_out (cli, "STATUS  : %s (%d), %s, %s \n", mod->state_name, mod->state, mod->name, mod->type);
        cli_out (cli, "MODULE  : %s / %s \n", mod->manufacturer, mod->info);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        cli_out (cli, "LEVEL   : %d% \n", mod->level);
        cli_out (cli, "VERSION : 4.6-150101-0\n");
        break;

   case PRODUCT_ANYDATA:
        cli_out (cli, "STATUS  : %s(%d), %s, %s, %s \n",
                    mod->state_name,
                    mod->state,
                    mod->name,
                    mod->type,
                    mod->apn_name);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "NUMBER2 : %s \n", mod->number2);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        cli_out (cli, "RUIMID  : %s \n", mod->ruimid);
        // ANYDATA 모듈의 버전에 따라 RSRP 값을 못 읽어오는 경우가 있다.
        // 이러한 경우에는 LEVEL 표기를 RSSI 로만 합니다.
        if (mod->rsrp == 0)
          cli_out (cli, "LEVEL   : -%d (RSSI) \n", mod->rssi);
        else
          cli_out (cli, "LEVEL   : %d% (%d) \n", mod->level, mod->rsrp);
        cli_out (cli, "M2MPCM  : N/A\n");
        break;

   case PRODUCT_LGM2M_TYPE1:
        cli_out (cli, "STATUS  : %s(%d), %s, %s, %s \n",
                    mod->state_name,
                    mod->state,
                    mod->name,
                    mod->type,
                    mod->apn_name);
        cli_out (cli, "NUMBER  : %s \n", mod->number);
        cli_out (cli, "NUMBER2 : %s \n", mod->number2);
        cli_out (cli, "IMEI    : %s \n", mod->imei);
        cli_out (cli, "LEVEL   : %d% (%d, %d) \n", mod->level, mod->rsrp, mod->sinr);
        if (is_lgm2m_type1_m2mpcm() == LTE_OK)
            cli_out (cli, "M2MPCM  : RUNNING\n");
        else
            cli_out (cli, "M2MPCM  : ERROR\n");
        cli_out (cli, "UICC    : %s \n", mod->lgu_uicc);
      break;

    case PRODUCT_LGM2M_TYPE2:
      if (is_ready_lgm2m_type2() == LTE_ERROR)
        {
          cli_out (cli, " lte0 interface must have ip address by DHCP before using show lte.\n");
          return;
        }

      if (mod->state_name[0] == 'I')
        {
          cli_out (cli,
                    "STATUS  : %s, %s, %s \n",
                    mod->state_name,
                    mod->type,
                    mod->apn_name);
        }
      else
        {
          cli_out (cli,
                    "STATUS  : DISCONNECTED (%s), %s, %s \n",
                    mod->state_name,
                    mod->type,
                    mod->apn_name);
        }

      cli_out (cli, "NUNBER  : %s \n", mod->number);
      cli_out (cli, "IMEI    : %s \n", mod->imei);

      // rssi 값이 99이면 No Service 상태로 표시합니다.
      if (mod->rssi == 99)
        cli_out (cli, "LEVEL   : NO SERVICE (%d)\n", mod->level, mod->rssi);
      else
        cli_out (cli, "LEVEL   : %d% (%d)\n", mod->level, mod->rssi);
      break;

    case PRODUCT_PEGASUS:
      if (mod->state_name[0] == 'I')
        {
          cli_out (cli,
                    "STATUS  : %s, %s, %s \n",
                    mod->state_name,
                    mod->type,
                    mod->apn_name);
        }
      else
        {
          cli_out (cli,
                    "STATUS  : DISCONNECTED (%s), %s, %s \n",
                    mod->state_name,
                    mod->type,
                    mod->apn_name);
        }

      cli_out (cli, "NUNBER  : %s \n", mod->number);
      cli_out (cli, "IMEI    : %s \n", mod->imei);

      if (mod->level == 0)
        cli_out (cli, "LEVEL   : NO SERVICE (%d)\n", mod->level, mod->rsrp);
      else
        cli_out (cli, "LEVEL   : %d% (%d)\n", mod->level, mod->rsrp);
      break;

    default:
      break;
  }

  cli_out (cli, "IFACE   : %s, %s\n", mod->if_type, mod->ip_addr);
}



/* Comment ********************************************************************
 * lte 로 사용되는 IP를 얻어옵니다.
 *
 */
static char *
_lte_find_ip_address (char *name)
{
  int fd;
  struct ifreq ifrq;
  struct sockaddr_in *sin;

  fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    return "N/A";

  memset(&ifrq, 0, sizeof(struct ifreq));
  strcpy (ifrq.ifr_name, name);

  if(ioctl(fd, SIOCGIFADDR, &ifrq) < 0)
    {
      close(fd);
      return "N/A";
    }

  sin = (struct sockaddr_in *)&ifrq.ifr_addr;
  close(fd);

  return inet_ntoa(sin->sin_addr);
}


/* Comment ********************************************************************
 * lte 로 사용되는 모듈을 체크합니다.
 *
 */
static int
_lte_find_interface (char *name)
{
  int fd;
  struct ifreq ifrq;

  fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    return LTE_ERROR;

  memset(&ifrq, 0, sizeof(struct ifreq));
  strcpy (ifrq.ifr_name, name);

  if(ioctl(fd, SIOCGIFHWADDR, &ifrq) < 0)
    {
      close(fd);
      return LTE_ERROR;
    }

  close(fd);

  return LTE_OK;
}


/* Comment ********************************************************************
 * 인터페이스 정보를 추가로 얻어오도록 합니다. 
 * LTE 가 사용중인 인터페이스 이름과 IP를 표기하도록 합니다. 
 *
 */
void
lte_get_interface_info (ZEBOS_LTE_MODULE_t *module)
{

  if (_lte_find_interface("lte0") == LTE_OK)
    {
      pal_snprintf(module->if_type, sizeof(module->if_type), "lte0");
      pal_snprintf(module->ip_addr, sizeof(module->ip_addr), _lte_find_ip_address("lte0"));
    }
  else if (_lte_find_interface("ltepdn0") == LTE_OK)
    {
      pal_snprintf(module->if_type, sizeof(module->if_type), "ltepdn0");
      pal_snprintf(module->ip_addr, sizeof(module->ip_addr), _lte_find_ip_address("ltepdn0"));
    }
  else if (_lte_find_interface("ppp0") == LTE_OK)
    {
      pal_snprintf(module->if_type, sizeof(module->if_type), "ppp0");
      pal_snprintf(module->ip_addr, sizeof(module->ip_addr), _lte_find_ip_address("ppp0"));
    }   
  else
    {
      pal_snprintf(module->if_type, sizeof(module->if_type), "N/A");
      pal_snprintf(module->ip_addr, sizeof(module->ip_addr), "N/A");
    }   



}


/* Comment *******************************************************
 * LTE 정보를 얻어옵니다.
 *
 */
int
lte_get_module_info (ZEBOS_LTE_MODULE_t *module)
{
  int product = PRODUCT_NONE;

  // LTE 모듈의 연결상태를 확인합니다.
  product = lte_get_product();
  if (PRODUCT_NONE == product)
    return LTE_ERROR;

  switch(product)
  {
    case PRODUCT_AMT:
      strcpy(module->manufacturer, "AMTelecom");
      get_amp5210_status(module);
      break;

    case PRODUCT_KNI:
      strcpy(module->manufacturer, "Kernel-I");
      get_kmkl200_status(module);
      break;

    case PRODUCT_MEC:
      strcpy(module->manufacturer, "MobileECO");
      get_mec_status(module);
      break;

    case PRODUCT_NT:
      strcpy(module->manufacturer, "NTmore");
      get_ntmore_status(module);
      break;

    case PRODUCT_NTLM200Q:
      strcpy(module->manufacturer, "NTmore");
      get_ntlm200q_status(module);
      break;

    case PRODUCT_L300S:
      strcpy(module->manufacturer, "Pantech");
      get_l300s_status(module);
      break;

    case PRODUCT_TM400:
      strcpy(module->manufacturer, "Teladin");
      get_tm400_status(module);
      break;

    case PRODUCT_ANYDATA:
      strcpy(module->manufacturer, "Anydata");
      get_anydata_status(module);
      break;

    case PRODUCT_LGM2M_TYPE1:
      strcpy(module->manufacturer, "LG-M2M");
      get_lgm2m_type1_status(module);
      break;

    case PRODUCT_LGM2M_TYPE2:
      strcpy(module->manufacturer, "LG-M2M RNDIS");
      get_lgm2m_type2_status(module);
      break;

    case PRODUCT_PEGASUS:
      strcpy(module->manufacturer, "LG-M2M PEGASUS");
      get_lgm2m_pegasus_status (module);
      break;

    default :
      return LTE_ERROR;
  }

  // 인터페이스 정보를 추가로 얻어오도록 합니다.
  lte_get_interface_info (module);

  return LTE_OK;
}


/* Comment ********************************************************************
 * KT IOT 에 필요한 정보를 보여줍니다.
 *
 */
void
lte_show_status_detail (struct cli *cli)
{
  int product = 0;

  product = lte_get_product ();

  if (product == PRODUCT_KNI)
    get_kmkl200_detail_status (cli);
  else if (product == PRODUCT_NTLM200Q)
    get_ntlm200q_detail_status (cli);
  else
    cli_out (cli, "'show lte detail' CLI not available.\n");
}


/* Comment *******************************************************
 * LTE 모듈을 통해 문자메시지를 전송합니다.
 *
 */
void
lte_send_sms (struct cli *cli, char *pNumber, char *pMsg)
{
  int ret = 0;
  int product = PRODUCT_NONE;
  ZEBOS_LTE_MODULE_t module;

  // LTE 모듈의 연결상태를 확인합니다.
  product = lte_get_product ();
  if (PRODUCT_NONE == product)
    {
      cli_out (cli, "%%LTE: Not found LTE Module.\n");
    return;
    }

  memset (&module, 0x00, sizeof(module));

  switch (product)
    {
    case PRODUCT_AMT:
      strcpy (module.manufacturer, "AMTelecom");
      ret = sendmsg_amp5210 (cli, &module, pNumber, pMsg);
      break;

    case PRODUCT_KNI:
      strcpy (module.manufacturer, "Kernel-I");
      ret = sendmsg_kmkl200 (cli, &module, pNumber, pMsg);
      break;

    case PRODUCT_MEC:
      strcpy (module.manufacturer, "MobileECO");
      ret = sendmsg_mec (cli, &module, pNumber, pMsg);
      break;

    case PRODUCT_NTLM200Q:
      strcpy (module.manufacturer, "NTmore");
      ret = sendmsg_ntlm200q (cli, &module, pNumber, pMsg);
      break;

    case PRODUCT_L300S:
      strcpy (module.manufacturer, "Pantech");
      ret = sendmsg_l300s (cli, &module, pNumber, pMsg);
      break;
    }

  if (ret == LTE_OK)
    cli_out (cli, "%%LTE: \"%s\", \"%s\" \nSend message OK.\n", pNumber, pMsg);
  else
    cli_out (cli, "%%LTE: \"%s\", \"%s\" \nSend message Error.\n", pNumber, pMsg);
}


/* Comment ********************************************************************
 * 사용자로부터 at command를 받아서 출력합니다.
 *
 */
void
lte_send_serial (char *cmd, char *pbuf)
{
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  lte_xmit_serial(cmd, pbuf);
}


/* Comment ********************************************************************
 *  OTA From SKT.
 *  정상적으로 처리가 되면 0을 리턴합니다.
 */
int
lte_ota_from_skt (struct cli *cli)
{
  int ret = 0;
  int product = PRODUCT_NONE;
  ZEBOS_LTE_MODULE_t module;

  product = lte_get_product ();
  memset (&module, 0x00, sizeof(module));
  switch (product)
    {
    case PRODUCT_AMT:
      ret = set_amp5210_ota (OTA_TYPE_SKT);
      cli_out (cli, "! LTE: OTA From SKT\n");
      break;

    case PRODUCT_KNI:
      ret = set_kmkl200_ota (OTA_TYPE_SKT);
      cli_out (cli, "! LTE: OTA From SKT\n");
      break;

    case PRODUCT_NTLM200Q:
      ret = set_ntlm200q_ota (OTA_TYPE_SKT);
      cli_out (cli, "! LTE: OTA From SKT\n");
      break;

    case PRODUCT_L300S:
      ret = set_l300s_ota (OTA_TYPE_SKT);
      cli_out (cli, "! LTE: OTA From SKT\n");
      break;

    default:
      return LTE_ERROR;
    }

  return ret;
}


/* Comment ********************************************************************
 *  OTA From KT.
 *  정상적으로 처리가 되면 0을 리턴합니다.
 */
int
lte_ota_from_kt (struct cli *cli)
{
  int ret = 0;
  int product = PRODUCT_NONE;
  ZEBOS_LTE_MODULE_t module;

  product = lte_get_product ();
  memset (&module, 0x00, sizeof(module));
  switch (product)
    {
    case PRODUCT_AMT:
      ret = set_amp5210_ota (OTA_TYPE_KT);
      cli_out (cli, "! LTE: OTA From KT\n");
      break;

    case PRODUCT_KNI:
      ret = set_kmkl200_ota (OTA_TYPE_KT);
      cli_out (cli, "! LTE: OTA From KT\n");
      break;

    case PRODUCT_TM400:
      ret = set_tm400_ota (OTA_TYPE_KT);
      cli_out (cli, "! LTE: OTA From KT\n");
      break;

    default:
      return -1;
    }

  return ret;
}


/* Comment ********************************************************************
 * APN을 변경하기 위한 AT commmnad 를 동작시킵니다.
 *
 */
int
lte_change_apn (int type, char *apn_url)
{
  int ret = LTE_ERROR;
  int product = PRODUCT_NONE;

  product = lte_get_product ();
  switch (product)
    {
    case PRODUCT_KNI:
      // KNI 모듈은 KT, SKT 만 지원합니다.
      if (type == APN_TYPE_LGU)
        return LTE_ERROR;

      ret = set_kmkl200_apn (type, apn_url);
      break;

    case PRODUCT_AMT:
      // AMT 모듈은 KT, SKT 만 지원합니다.
      if (type == APN_TYPE_LGU)
        return LTE_ERROR;

      ret = set_amp5210_apn (type, apn_url);
      break;

    case PRODUCT_NTLM200Q:
      // NTLM200Q 모듈은 SKT만 존재합니다.
      if (type == APN_TYPE_SKT || APN_TYPE_DEFAULT)
        ret = set_ntlm200q_apn (type, apn_url);
      break;

    case PRODUCT_L300S:
      // NTLM200Q 모듈은 SKT만 존재합니다.
      if (type == APN_TYPE_SKT || APN_TYPE_DEFAULT)
        ret = set_l300s_apn (type, apn_url);
      break;

    case PRODUCT_TM400:
      // TM400 모듈은 KT만 존재합니다.
      if (type == APN_TYPE_KT || APN_TYPE_DEFAULT || APN_TYPE_USER)
        ret = set_tm400_apn (type, apn_url);
      break;

    case PRODUCT_ANYDATA:
      // ANYDATA 모듈은 LGU만 존재합니다.
      if (type == APN_TYPE_LGU || APN_TYPE_DEFAULT)
        ret = set_anydata_apn (type, apn_url);
      break;

    case PRODUCT_LGM2M_TYPE1:
      // LGM2M 모듈은 LGU만 존재합니다.
      if (type == APN_TYPE_LGU || APN_TYPE_DEFAULT)
        ret = set_lgm2m_type1_apn (type, apn_url);
      break;

    case PRODUCT_LGM2M_TYPE2:
      // LGM2M 모듈은 LGU만 존재합니다.
      if (type == APN_TYPE_LGU || APN_TYPE_DEFAULT)
        ret = set_lgm2m_type2_apn (type, apn_url);
      break;

    case PRODUCT_PEGASUS:
      // LGM2M 모듈은 LGU만 존재합니다.
      if (type == APN_TYPE_LGU || APN_TYPE_DEFAULT)
        ret = set_lgm2m_pegasus_apn (type, apn_url);
        break;

      default:
      return LTE_ERROR;
    }

  return ret;
}


/* Comment ********************************************************************
 * APN애 관한 정보를 자세하게  표기합니다.
 *
 */
void
lte_show_apn_detail (struct cli *cli)
{
  int product = 0;
  lte_apn_info_t apn_info;

  product = lte_get_product ();
  memset (&apn_info, 0x00, sizeof(apn_info));

  if (product == PRODUCT_KNI)
    read_kmkl200_apn (&apn_info);
  else if (product == PRODUCT_AMT)
    read_amp5210_apn (&apn_info);
  else if (product == PRODUCT_ANYDATA)
    read_anydata_apn (&apn_info);
  else if (product == PRODUCT_LGM2M_TYPE1)
    read_lgm2m_type1_apn (&apn_info);
  else if (product == PRODUCT_LGM2M_TYPE2)
    read_lgm2m_type2_apn (&apn_info);
  else if (product == PRODUCT_PEGASUS)
    read_lgm2m_pegasus_apn (&apn_info);
  else if (product == PRODUCT_NTLM200Q)
    read_ntlm200q_apn (&apn_info);
  else if (product == PRODUCT_L300S)
    read_l300s_apn (&apn_info);
  else if (product == PRODUCT_TM400)
    read_tm400_apn (&apn_info);
  else
    {
      cli_out (cli, "%% Not found LTE Module - !! \n");
      return;
    }

  cli_out (cli, "APN : %s \n", apn_info.short_apn);
  cli_out (cli, "      %s \n", apn_info.long_apn);
}


/* Comment ********************************************************************
 * APN에 관한 정보를 표기합니다.
 *
 */
void
lte_show_apn (struct cli *cli)
{
  int product = 0;
  lte_apn_info_t apn_info;

  product = lte_get_product ();
  memset (&apn_info, 0x00, sizeof(apn_info));

  if (product == PRODUCT_KNI)
    read_kmkl200_apn (&apn_info);
  else if (product == PRODUCT_AMT)
    read_amp5210_apn (&apn_info);
  else if (product == PRODUCT_ANYDATA)
    read_anydata_apn (&apn_info);
  else if (product == PRODUCT_LGM2M_TYPE1)
    read_lgm2m_type1_apn (&apn_info);
  else if (product == PRODUCT_LGM2M_TYPE2)
    read_lgm2m_type2_apn (&apn_info);
  else if (product == PRODUCT_PEGASUS)
    read_lgm2m_pegasus_apn (&apn_info);
  else if (product == PRODUCT_NTLM200Q)
    read_ntlm200q_apn (&apn_info);
  else if (product == PRODUCT_L300S)
    read_l300s_apn (&apn_info);
  else if (product == PRODUCT_TM400)
    read_tm400_apn (&apn_info);
  else
    {
      cli_out (cli, "%% Not found LTE Module - !! \n");
      return;
    }

  cli_out (cli, "APN : %s \n", apn_info.short_apn);
}

/* Comment ********************************************************************
 * config 파일들을 초기화 하기 위하여 M2MG의 default 설정을 복사해옵니다.
 * Anydata 모듈의 경우에는 ppp로 접속되기 때문에 다른 설정 파일를 사용하도록합니다.
 *
 */
static void
_lte_copy_default_config (void)
{
  if ( 0 == access ("/media/disk0/startup-config", R_OK))
    {
      system ("rm -rf /media/disk0/startup-config");
      system ("sync");
      system ("sync");
      fprintf (stderr, "DIDO: remove startup-config file \r\n");
    }

  if (lte_get_product() == PRODUCT_ANYDATA)
    {
      // 초기화에 필요한 코드를 추가하면 됩니다.
      if ( 0 == access ("/etc/m2m_conf/M2M_CONF.ppp", R_OK))
        {
          system ("cp /etc/m2m_conf/M2M_CONF.ppp /media/disk0/M2M_CONF");
          system ("sync");
          fprintf (stderr, "DIDO: initialize M2M_CONF file \r\n");
        }
      if ( 0 == access ("/etc/m2m_conf/startup-config.ppp", R_OK))
        {
          system ("cp /etc/m2m_conf/startup-config.ppp /media/disk0/startup-config");
          system ("sync");
          fprintf (stderr, "DIDO: initialize startup-config file \r\n");
        }
    }
  else
    {
      // 초기화에 필요한 코드를 추가하면 됩니다.
      if ( 0 == access ("/etc/m2m_conf/M2M_CONF.0", R_OK))
        {
          system ("cp /etc/m2m_conf/M2M_CONF.0 /media/disk0/M2M_CONF");
          system ("sync");
          fprintf (stderr, "DIDO: initialize M2M_CONF file \r\n");
        }
      if ( 0 == access ("/etc/m2m_conf/startup-config.0", R_OK))
        {
          system ("cp /etc/m2m_conf/startup-config.0 /media/disk0/startup-config");
          system ("sync");
          fprintf (stderr, "DIDO: initialize startup-config file \r\n");
        }
    }

#ifdef HAVE_LOCKOUT
  sqlite3 *pDb;
  char query_buf[BUFSIZ];

  sqlite3_open(USER_DB_PATH, &pDb);
  if (pDb != NULL)
    {
      sqlite3_exec(pDb, USER_DB_CREATE_TABLE_STR, NULL, NULL, NULL);

	  pal_snprintf(query_buf, sizeof(query_buf), "update account_t set failcount=0");
      sqlite3_exec(pDb, query_buf, NULL, NULL, NULL);

      sqlite3_close(pDb);
    }
#endif /* HAVE_LOCKOUT */

  fprintf (stderr, "DIDO: reboot system..!! \r\n");
  fflush(stderr);
  sleep(1);
  system ("reboot -f");
}


/* Comment ********************************************************************
 * Get factory reset signal state
 *
 */
void
lte_check_factoryreset (void)
{
  int read_state = 0;
  static int lte_fac_count = 0;

  read_state = _lte_read_sysM2M (CMD_SYSM2M_FAC_READ);

  if (read_state == LTER_FAC_NONE)
    {
      lte_fac_count = 0;
      return;
    }

  // 상태값이 FAC_RESET 인 경우에만 동작되도록 합니다.
  if (read_state == LTER_FAC_RESET)
    {
      if (lte_fac_count++ >= 2)
        _lte_copy_default_config ();
      return;
    }
  else
    {
      lte_fac_count = 0;
      return;
    }
}

/* Comment ********************************************************************
 * lte0 인터페이스가 생성이 되었는지 확인합니다.
 *
 */
int
lte_is_available (void)
{
  int ret;

  // ppp0 를 사용하는 ANYDATA 모듈을 체크합니다.
  if (lte_get_product() == PRODUCT_ANYDATA)
    return LTE_OK;

  ret = _lte_find_interface("lte0");
  if (ret == LTE_OK)
    return LTE_OK;

  ret = _lte_find_interface("ltepdn0");
  if (ret == LTE_OK)
    return LTE_OK;

  return LTE_ERROR;
}


/* Comment ********************************************************************
 * LTE 모듈의 상태가 "IN SERVCIE" 인지를 확인합니다.
 *
 */
int
lte_is_network_service_available (void)
{
  int product = 0;

  product = lte_get_product ();

  // lte / ltepdn 인터페이스가 생성되지 않으면 LTE_ERROR 를 리턴합니다.
  if (lte_is_available() == LTE_ERROR)
    return LTE_ERROR;

  // 테스트 하지 못한 모듈은 LTE_ERROR로 리턴하도록 합니다.
  if (product == PRODUCT_KNI)
    return read_kmkl200_network_status ();
  else if (product == PRODUCT_ANYDATA)
    return read_anydata_network_status ();
  else if (product == PRODUCT_LGM2M_TYPE1)
    return read_lgm2m_type1_network_status ();
  else if (product == PRODUCT_LGM2M_TYPE2)
    return read_lgm2m_type2_network_status ();
  else if (product == PRODUCT_NTLM200Q)
    return read_ntlm200q_network_status ();
  else if (product == PRODUCT_L300S)
    return read_l300s_network_status ();
  else if (product == PRODUCT_TM400)
    return read_tm400_network_status ();
  else if (product == PRODUCT_PEGASUS)
    return read_lgm2m_pegasus_network_status ();
  else
    return LTE_ERROR;
}


/* Comment *******************************************************
 * 등록되어 있는 driver를 체크합니다.
 *
 */
int
lte_is_driver (char *pCmd)
{
  FILE    *fp = NULL;
  int     read_size = 0;
  char    buf[LTE_BUF_LENGTH];

  fp = popen(pCmd, "r");

  if (!fp)
    return LTE_ERROR;

  memset(buf, 0x00, sizeof(buf));
  read_size = fread( (void*)buf, sizeof(char), LTE_BUF_LENGTH - 1, fp );

  pclose(fp);

  if (read_size > 0)
      return LTE_OK;
  else
    return LTE_ERROR;
}


/* Comment ********************************************************************
 * LTE 모듈의 level 값을 리턴합니다.
 *
 */
int
lte_get_level (void)
{
  ZEBOS_LTE_MODULE_t module = {};

  if (lte_get_module_info(&module) == LTE_OK)
    return module.level;
  else
    return 0;
}


/* Comment ********************************************************************
 * LTE 모듈의 rmnet 상태를 enable 시킵니다.
 * NTLM-200Q 모듈만 동작하도록 합니다.
 *
 */
int
lte_enable_rmnet(void)
{
  int product = 0;

  product = lte_get_product ();
  if (product == PRODUCT_NTLM200Q)
    return enable_ntlm200q_rmnet ();
  else
    return LTE_OK;
}


#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
