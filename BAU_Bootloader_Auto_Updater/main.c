#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "type.h"
#include "message.h"
#include "use_mtd0.h"

#define BUF_SIZE                        128
#define STRING_LENGTH                   128
#define STAMP_LENGTH                    32

#define OK                              0
#define ERROR                           -1

#define STATE_INIT                      0
#define STATE_CHK_MACHIN_BOOTLOADER     1
#define STATE_DOWDLOAD_SVR_BOOTLOADER   2
#define STATE_COMPARE_BOOTLOADER        3
#define STATE_ASK_BOOTLOADER_UPDATE     4
#define STATE_BOOTLOADER_UPDATE         5
#define STATE_NORMAL_EXIT               6
#define STATE_UPDATE_COMPLETE           7
#define STATE_ERROR_EXIT                8

/* Comment ********************************************************************
 * bootloader의 tag 정보를 얻어옵니다. 
 *
 */
static int
get_bootloader_tag (char *pfile, struct stamp_tag *psrc)
{
  int fd;
  char buf[STRING_LENGTH + 1];
  char *ptr;
  int index = 0;

  fd = open (pfile, O_RDONLY);

  // 부트로더 정보를 읽어오기 위하여
  // 마지막 32 byte을 읽어오도록 합니다.
  memset (buf, 0x00, sizeof(buf));
  lseek (fd, (STAMP_LENGTH * -1), SEEK_END);
  read(fd, buf, STRING_LENGTH);

  close (fd);

  // 읽어온 부트로더 정보를 처리합니다.
  ptr = strtok (buf, " ");
  index++;

  // GIT TAG 가 안 맞으면 에러로 처리합니다.
  if (strncmp (ptr, GIT_TAG, 3) != 0)
    return ERROR;

  while ((ptr = strtok(NULL, " ")) != NULL)
  {
    if (index == 1)
      {
        snprintf (psrc->ver, sizeof(psrc->ver), "%s", ptr);
      }
    else if (index == 2)
      {
        if (strncmp (ptr, STAMP_TAG, 1) != 0)
          return ERROR;
      }
    else if (index == 3)
      {
        snprintf (psrc->stamp, sizeof(psrc->ver), "%s", ptr);
      }
    else if (index == 4)
      {
        if (strncmp (ptr, END_TAG, 1) != 0)
          return ERROR;
      }

    index++;
  }

  return OK;
}


#if 0
static
int is_update (struct stamp_tag *psrc, struct stamp_tag *ptarget)
{
  int src_stamp = 0;
  int target_stamp = 0;

  src_stamp = atoi(psrc->stamp);
  target_stamp = atoi(ptarget->stamp);

  // 이미지의 stamp 를 검사합니다.
  // 업데이트하고자 하는 이미지의 stamp 값이
  // 장비 이미지의 stamp 값보다 작거나 같으면,
  // 최신의 이미지가 이미 적용되었기 때문에
  // 부트로더를 업데이트 하지 않습니다.
  if (src_stamp <= target_stamp)
    return ERROR;

  return OK;
}


int main(int argc, char *argv[])
{
  struct stamp_tag src_img;
  struct stamp_tag target_img;

  printf ("Checking bootloader update file...  \r\n" );


  // 업데이트를 할 파일을 찾습니다.
  if (0 != access(VF0500_MASTER_IMAGE, F_OK))
    {
      printf ("Sorry, Not found bootloader image.\r\n" );
      return 0;
    }
  else
    {
      printf ("Found bootloader image.\r\n" );
    }



  // 업데이트 할 이미지의 정보를 가지고 온다.
  memset (&src_img, 0x00, sizeof(src_img));
  if (get_version_and_stamp (VF0500_MASTER_IMAGE, &src_img) == OK)
    {
      printf ("Find bootloader image (GIT:%s T:%s) \r\n", src_img.ver, src_img.stamp);
    }
  else
    {
      printf ("Sorry, Not found build number of bootloader image.\r\n" );
      goto error_return;
    }

  // 장비의 부트로더 이미지 정보를 가지고 온다.
  // 부트로더 이미지를 가지고 오지 못하면
  // 부트로더가 예전 것 이라고 판단하고 업데이트를 진행합니다.
  memset (&target_img, 0x00, sizeof(target_img));
  if (get_version_and_stamp (TARGET_IMAGE, &target_img) == OK)
    {
      printf ("Find target image (GIT:%s T:%s) \r\n",
              target_img.ver, target_img.stamp);

      // 업데이트 여부를 확인합니다.
      if (is_update (&src_img, &target_img) == OK)
        {
          printf ("Bootloader update \r\n");
        }
      else
        {
          printf ("GIT   : %s \r\n", target_img.ver);
          printf ("STAMP : %s \r\n", target_img.stamp);
          printf ("Your image is up to date. \r\n");
        }
    }
  else
    {
      printf ("Bootloader update \r\n");
    }

  return OK;

error_return :
  return ERROR;
}
#endif



/* Comment ********************************************************************
 * 장비의 mtd 블럭이 준비가 되어 있는 지 확인한다.
 *
 */
static int
is_ready_mtdblock (void)
{
  FILE *fp = NULL;
  char buf[BUF_SIZE + 1];
  char cmd[BUF_SIZE + 1];
  char copy_cmd[BUF_SIZE + 1];
  int ret = 0;

  if (0 != access (SYS_MTD1_NAME_PATH, R_OK))
    return ERROR;

  if (0 != access (DEV_MTD1_PATH, R_OK))
    return ERROR;

  memset (cmd, 0x00, sizeof(cmd));
  memset (buf, 0x00, sizeof(buf));
  sprintf(cmd, "cat %s | grep -c 'uboot'", SYS_MTD1_NAME_PATH);
  fp = popen(cmd, "r");

  if (!fp)
    return -1;
  else
    fgets( buf, BUF_SIZE, fp);

  ret = atoi(buf);

  pclose(fp);

  if (ret == 1)
    {
      memset (copy_cmd, 0x00, sizeof(copy_cmd));
      sprintf (copy_cmd, 
              "dd if=%s of=%s conv=sync 2> /dev/null",
              DEV_MTD1_PATH,
              TMP_UBOOT_PATH);
      system (copy_cmd);
    }

  if (0 == access (TMP_UBOOT_PATH, R_OK))
    return OK;
  else
    return ERROR;
}


/* Comment ********************************************************************
 * 현재 장비의 booad_type 값을 리턴합니다.
 *
 */
static int
get_board_type (void)
{
  FILE *fp = NULL;
  char buf[BUF_SIZE + 1];
  int board_type = 0;

  memset (buf, 0x00, sizeof(buf));
  if ((fp = fopen ("/proc/octeon_info", "r")))
    {
      while (fgets (buf, BUFSIZ, fp))
        {
          if (!strncmp (buf, "board_type", strlen ("board_type")))
            {
              sscanf (buf, "board_type: %d\n", &board_type);
              break;
            }
        }

      fclose (fp);
    }

  return board_type;
}


/* Comment ********************************************************************
 * 다운받은 부트로더 path 를 설정합니다. 
 *
 */
static void
convert_bootloader_path(int board_type, char *bootloader_path)
{
  int i = 0;
  int list_max = (sizeof(boot_list)/sizeof(struct model_list));

  for (i = 0; i < list_max; i++)
    {
      if (boot_list[i].board_type == board_type)
        {
          sprintf(bootloader_path, "/media/disk0/%s", boot_list[i].bootloader_name);
          break;
        }
    }
}


/* Comment ********************************************************************
 * FTP 서버에서 부트로더 이미지를 가지고 온다.
 *
 */
static int
is_download_bootloader(int board_type, char *bootloader_path)
{
  FILE *fp = NULL;
  int i = 0;
  char buf[BUF_SIZE + 1];
  char cmd[BUF_SIZE + 1];
  int list_max = (sizeof(boot_list)/sizeof(struct model_list));

  memset (cmd, 0x00, sizeof(cmd));
  memset (buf, 0x00, sizeof(buf));

  for (i = 0; i < list_max; i++)
    {
      if (boot_list[i].board_type == board_type)
        {
          sprintf (cmd, 
                   "copy %s%s %s -f", 
                   UPDATE_SERVER_PATH,
                   boot_list[i].bootloader_name,
                   bootloader_path);
          fp = popen(cmd, "r");

          if (!fp)
            return -1;
          else
            fgets( buf, BUF_SIZE, fp);

          pclose(fp);
          break;
        }
    }

  if (0 == access (bootloader_path, R_OK))
    return OK;
  else
    return ERROR;
}



/* Comment ********************************************************************
 * 커널에 mtdutils 이 설치되어 있는 지 확인합니다.
 *
 */
static int
is_ready_mtdutils (void)
{
  if (0 == access (FLASH_ERASEALL_BIN, R_OK))
    return OK;
  else
    return ERROR;
}



/* Comment ********************************************************************
 * 부트로더 파일이 존재한다면 업데이트를 진행합니다. 
 *
 */
static int
update_bootloader (char *bootloader_path)
{
  char cmd[BUF_SIZE + 1];
  
  if (0 == access (bootloader_path, R_OK))
    {
      memset (cmd, 0x00, sizeof(cmd));
      sprintf (cmd, 
              "%s %s", 
              FLASH_ERASEALL_BIN,
              DEV_MTD1_PATH);
      system (cmd);

      memset (cmd, 0x00, sizeof(cmd));
      sprintf (cmd, 
              "dd if=%s of=%s conv=sync 2> /dev/null", 
              bootloader_path,
              DEV_MTD1_PATH);
      system (cmd);
      return OK;
    }
  else
    {
      return ERROR;
    }
}


/* Comment ********************************************************************
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  int state = STATE_INIT;
  int dev_stamp;
  int download_stamp;
  int board_type;
  char bootloader_path[BUF_SIZE];
  struct stamp_tag device_bootimg;
  struct stamp_tag download_bootimg;

  state = STATE_INIT;
  while (1)
    {
      switch(state)
        {
          /* state 머신을 초기화합니다. */
          case STATE_INIT:
            STATE_PRINT ("Start Bootloader Auto Updater");
            state = STATE_CHK_MACHIN_BOOTLOADER;
            break;

          /* 장비에 설치되어 있는 부트로더 이미지를 확인합니다.     */
          /* mtdblock이 준비되어 있지 않으면 ERROR.                 */
          case STATE_CHK_MACHIN_BOOTLOADER:
            STATE_PRINT ("Check Bootloader version.");
            // mtdblock이 준비 되었는 지 확인합니다.
            if (is_ready_mtdblock () == ERROR)
              {
                ERR_PRINT (ERR_NOT_FOUND_MTDBLOCK);
                state = STATE_ERROR_EXIT;
                break;
              }

            // mtdutils 패키지가 준비되었는지 확인합니다.
            if (is_ready_mtdutils () == ERROR)
              {
                ERR_PRINT (ERR_NOT_FOUND_MTDUTILS);
                state = STATE_ERROR_EXIT;
                break;
              }

            // /tmp/uboot.bin 으로 복사된 이미지에서 tag 메시지를 찾습니다.
            // tag 메시지를 찾지 못하면 오래된 부트로더로 인식합니다.
            // 그런 경우에는 에러메시지를 출력하고 부트로더를 다운받도록 합니다.
            memset (&device_bootimg, 0x00, sizeof(device_bootimg));
            if (get_bootloader_tag (TMP_UBOOT_PATH, &device_bootimg) == ERROR)
              {
                NOTICE_PRINT (NOTI_NOT_FOUND_TAG);
                state = STATE_DOWDLOAD_SVR_BOOTLOADER;
                break;
              }

            state = STATE_DOWDLOAD_SVR_BOOTLOADER;
            break;

          /* FTP 서버로 부터 이미지를 다운로드 받습니다.                    */
          /* FTP로 다운받지 못하거나, 받은 이미지의 tag 정보가 없으면 ERROR */
          case STATE_DOWDLOAD_SVR_BOOTLOADER:
            STATE_PRINT ("Download bootloader from FTP server");

            // 장비의 모델명과 업데이트할 u-boot의 이름을 설정합니다. 
            board_type = 0;
            memset (&bootloader_path, 0x00, sizeof(bootloader_path));
            board_type = get_board_type ();
            convert_bootloader_path (board_type, bootloader_path);

            // 부트로더를 ftp 서버로 부터 다운로드 받도록 합니다.
            // 이미지를 다운로드 받지 못하면 error 판단합니다.
            if (is_download_bootloader (board_type, bootloader_path) == ERROR)
              {
                ERR_PRINT (ERR_BOOTLOADER_DOWNLOAD_FAILED);
                state = STATE_ERROR_EXIT;
                break;
              }

            // 다운로드 받은 부트로더 이미지의 tag 정보를 가지고 입니다.
            // tag 정보를 가지고 오지 못하면 error 로 판단합니다.
            memset (&download_bootimg, 0x00, sizeof(download_bootimg));
            if (get_bootloader_tag (bootloader_path, &download_bootimg) == ERROR)
              {
                ERR_PRINT (ERR_NOT_FOUND_TAG);
                state = STATE_ERROR_EXIT;
                break;
              }

            state = STATE_COMPARE_BOOTLOADER;
            break;

          /* 부트로더 이미지의 tag 를 비교합니다. */
          case STATE_COMPARE_BOOTLOADER:
            STATE_PRINT ("Compare Bootloader version.");

            printf ("  Device bootloader tag (GIT:%s T:%s) \r\n",
              device_bootimg.ver, device_bootimg.stamp);
            printf ("Download bootloader tag (GIT:%s T:%s) \r\n",
              download_bootimg.ver, download_bootimg.stamp);

            dev_stamp = atoi (device_bootimg.stamp);
            download_stamp = atoi (download_bootimg.stamp);

            if (dev_stamp >= download_stamp)
              {
                NOTICE_PRINT (NOTI_LATEST_BOOTLOADER);
                state = STATE_NORMAL_EXIT;
                break;
              }
            else if (dev_stamp < download_stamp)
              {
                state = STATE_BOOTLOADER_UPDATE;
                break;
              }
            else
              {
                state = STATE_ERROR_EXIT;
                break;
              }

            state = STATE_ERROR_EXIT;
            break;

          /* 부트로더를 업데이트 합니다. */
          case STATE_BOOTLOADER_UPDATE:
            STATE_PRINT ("Bootloader update");

            // 부트로더 이미지를 mtd1에 복사합니다.
            if (update_bootloader (bootloader_path) == ERROR)
              {
                ERR_PRINT (ERR_NOT_FOUND_BOOTLOADER_PATH);
                state = STATE_ERROR_EXIT;
                break; 
              }

            state = STATE_UPDATE_COMPLETE;
            break;

          case STATE_NORMAL_EXIT:
            STATE_PRINT ("Normal Exit. \r\n");
            exit (0);
            break;

          case STATE_UPDATE_COMPLETE:
            NOTICE_PRINT ("Bootloader Update Complete !!!! ");
            exit (0);
            break;

          case STATE_ERROR_EXIT:
            STATE_PRINT ("Error Exit. \r\n");
            exit (0);
            break;
      }
    }
}


