#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

void console_print(const char *fmt, ...);

#define OK                              0
#define ERROR                           -1

#define MAX_CHECK_USB                   3

#define BUF_SIZE                        128
#define VERSION_TAG_LENGTH              8

#define MAX_UPDATE_COUNT                3

#define USB_PATH                        "/dev/sda1"
#define MOUNT_PATH                      "/media/usbdisk"
#define FLASH_ERASEALL_BIN              "/usr/sbin/flash_eraseall"
#define DEV_MTD4_PATH                   "/dev/mtd4"
#define SYS_MTD4_NAME_PATH              "/sys/block/mtdblock4/device/name"
#define TMP_IMAGE_PATH                  "/tmp/image.bin"
#define TMP_SPLIT_IMAGE_PATH            "/tmp/image_aa"
#define UPDATE_FILE_RULE                "OPENWRT_M2MG30S_"
#define UPDATE_OS_RULE                  "OPENWRT"
#define UPDATE_DEVICE_RULE              "M2MG30S"

#define ERROR_LINE         "*********************************************\r\n"
#define ERR_NOT_FOUND_MTDBLOCK          "Can't find mtdblock4 for image"
#define ERR_NOT_FOUND_MTDUTILS          "Can't find mtdutils"
#define ERR_BOOTLOADER_DOWNLOAD_FAILED  "Bootloader download failed"
#define ERR_NOT_FOUND_TAG               "Can't find the bootloader tag of download image"
#define ERR_NOT_FOUND_BOOTLOADER_PATH   "Can't find bootloader path"
#define ERR_NOT_FOUND_DEVCIE_VERSION    "Can't find device version"
#define ERR_NOT_FOUND_USBDISK           "Can't find usbdisk"
#define ERR_USBDISK_MOUNT               "USB Disk not mount."
#define ERR_COPY_IMAGE                  "MTDBlodk image invalid"
#define ERR_NOT_FOUND_FILE              "Can't find file"
#define ERR_PRINT(x) {console_print("\r\n%s %%ERR : %s \r\n\r\n", ERROR_LINE, x);}

#define NOTI_LATEST_BOOTLOADER         "Your Image is up to date."
#define NOTICE_PRINT(x) {console_print("\r\n# NOTI : %s \r\n\r\n", x);}


#define STATUS_LINE         "*********************************************\r\n"
#define STATUS_START                    "START UPDATING..."
#define STATUS_WAIT_10SEC               "WAIT 10 SEC"
#define STATUS_DEVICE_INIT              "DEVICE INIT OK"
#define STATUS_MTDBLOCK_COPY            "MTDBLOCK COPY OK"
#define STATUS_USBDISK_INIT             "USBDISK INIT OK"
#define STATUS_USBDISK_MOUNT            "USBDISK MOUNT OK"
#define STATUS_READY_MTDUTILS           "MTDUTILS OK"
#define STATUS_READY_FILE               "UPDATE FILE OK"
#define STATUS_ERASE_MTDBLOCK           "ERASE MTDBLOCK"
#define STATUS_WAIT_5MIN                "WAIT 5 MIN (FOR MTDBLOCK)"
#define STATUS_WRITE_IMAGE              "WRITE IMAGE"
#define STATUS_UPDATE_COMPLETE          "UPDATE COMPLETE"
#define STATUS_WAIT_60SEC               "WAIT 60 SEC (FOR INFORMATION)"
#define STATUS_PRINT(x) {console_print("# UPDATE : %s \r\n", x);}


#define STATUS_READY        0
#define STATUS_UPDATE       1
#define STATUS_COMPLETE     2
#define STATUS_ERROR        3

struct stamp_tag
{
  char tag[12];
  int stamp;
  char path[256];
  char sha1sum[128];
  int filesize;
};

int status_flag = STATUS_READY;


/* Comment ********************************************************************
 * printf 대신에 /dev/console로 바로 내보는 함수입니다.
 *
 */
void 
console_print(const char *fmt, ...)
{
  FILE *console;
  va_list args;
  char tmpbuf[1024];
  char printbuffer[1024];

  console = fopen("/dev/console", "a");

  va_start (args, fmt);

  /* For this to work, printbuffer must be larger than
 *    * anything we ever want to print.
 *       */
  vsprintf (tmpbuf, fmt, args);
  va_end (args);

  sprintf(printbuffer, "%s", tmpbuf);

  /* Send to desired file */
  fprintf(console, "%s", printbuffer);

  fclose(console);

  fflush(stdout);
}



/* Comment ********************************************************************
 * USB DISK가 인식이 되었는지 확인한다.
 *
 */
static int
access_usbdisk (void)
{
  if (access(USB_PATH, R_OK) == 0)
    return OK;
  else
    return ERROR;
}



/* Comment ********************************************************************
 * 마운트가 정상적으로 되었는지 확인한다.
 *
 */
static int
access_mount_path (void)
{
  if (access(MOUNT_PATH, R_OK) == 0)
    return OK;
  else
    return ERROR;
}



/* Comment ********************************************************************
 * MTD Block 의 이미지를 복사합니다.
 *
 */
static int
copy_mtdblock_image(struct stamp_tag *file_tag)
{
  FILE *fp = NULL;
  char buf[BUF_SIZE + 1];
  char cmd[BUF_SIZE + 1];
  char copy_cmd[BUF_SIZE + 1];
  int ret = 0;

  if (0 != access (SYS_MTD4_NAME_PATH, R_OK))
    return ERROR;

  if (0 != access (DEV_MTD4_PATH, R_OK))
    return ERROR;

  memset (cmd, 0x00, sizeof(cmd));
  memset (buf, 0x00, sizeof(buf));
  sprintf(cmd, "cat %s | grep -c 'kernel'", SYS_MTD4_NAME_PATH);
  fp = popen(cmd, "r");

  if (!fp)
    return -1;
  else
  {
    fgets (buf, BUF_SIZE, fp);
    buf[strlen(buf) - 1] = '\0';
  }

  ret = atoi(buf);

  pclose(fp);

  // MTD Block 이 정상적이라고 판단되면
  // USB로부터 복사한 사이즈만큼만 복사하도록 합니다.
  // 이는 SHA1SUM 값을 비교하기 위해서 입니다.
  if (ret == 1)
    {
      memset (copy_cmd, 0x00, sizeof(copy_cmd));
      sprintf (copy_cmd,
              "dd if=%s of=%s bs=1k conv=sync 2> /dev/null",
              DEV_MTD4_PATH,
              TMP_IMAGE_PATH,
              file_tag->filesize);
      system (copy_cmd);

      memset (copy_cmd, 0x00, sizeof(copy_cmd));
      sprintf (copy_cmd,
              "cd /tmp;split -b %d %s image_",
              file_tag->filesize,
              TMP_IMAGE_PATH);
      system (copy_cmd);
    }

  if (0 == access (TMP_IMAGE_PATH, R_OK))
    return OK;
  else
    return ERROR;
}


/* Comment ********************************************************************
 * Device에 설치된 OS의 정보를 얻어옵니다.
 *
 */
static int
init_device_info(struct stamp_tag *device_tag)
{
  char buf[256];
  char cmd[64];
  int find = 0;
  FILE *fp;
  char *ptr;

  memset(buf, 0x00, sizeof(buf));
  memset(cmd, 0x00, sizeof(cmd));

  // 버전 정보는 /proc/version 에 기록되어 있다.
  sprintf(cmd, "cat /proc/board_info  | grep  TimeStamp | awk '{print $5}'");
  fp = popen(cmd, "r");

  if (!fp)
    return ERROR;

  fgets(buf, sizeof(buf), fp);
  buf[strlen(buf) - 1] = '\0';

  if (pclose(fp) == -1)
    return ERROR;

  // GIT TAG의 길이는 8바이트로 고정되어 있다.
  ptr = strtok (buf, "._- ");
  if (strlen(ptr) == VERSION_TAG_LENGTH)
    memcpy(&device_tag->tag, ptr, VERSION_TAG_LENGTH);
  else
    return ERROR;

  // TIME STATMP 값은 항상 TAG 다음에 나온다.
  while ((ptr = strtok(NULL, "._- ")) != NULL)
  {
    device_tag->stamp = atoi(ptr);
    break;
  }

  return OK;
}



/* Comment ********************************************************************
 * USB가 올바로 준비되었는지 확인합니다.
 *
 */
static int
init_usbdisk_for_update (void)
{
  int i = 0;

  for (i = 0; i < MAX_CHECK_USB; i++)
  {
    // USB가 존재하는지 확인합니다.
    if (access_usbdisk() == OK)
      return OK;

    sleep(1);
  }

  return ERROR;
}



/* Comment ********************************************************************
 * 커널에 mtdutils 이 설치되어 있는 지 확인합니다.
 *
 */
static int
init_mtdutils (void)
{
  if (0 == access (FLASH_ERASEALL_BIN, R_OK))
    return OK;
  else
    return ERROR;
}


static int
init_file_path (char *src, struct stamp_tag *file)
{
  char path[128];
  char cmd[64];
  int find = 0;
  FILE *fp;

  memset(path, 0x00, sizeof(path));
  memset(cmd, 0x00, sizeof(cmd));

  sprintf(cmd, "pwd");
  fp = popen(cmd, "r");

  if (!fp)
    return ERROR;

  fgets(path, sizeof(path), fp);
  path[strlen(path) - 1] = '\0';

  if (pclose(fp) == -1)
    return ERROR;

  sprintf (file->path, "%s/%s", path, src);

  if (0 == access (file->path, R_OK))
    return OK;
  else
    return ERROR;
}



/* Comment ********************************************************************
 * USB를 /media/usbdisk로 마운트 합니다.
 *
 */
static int 
mount_usbdisk (void)
{
  char buf[64];
  char cmd[64];
  int find = 0;
  FILE *fp;

  memset(buf, 0x00, sizeof(buf));
  memset(cmd, 0x00, sizeof(cmd));

  sprintf(cmd, "mount | grep -c /media/usbdisk");
  fp = popen(cmd, "r");

  if (!fp)
    return ERROR;

  fgets(buf, sizeof(buf), fp);
  buf[strlen(buf) - 1] = '\0';

  find = atoi(buf);

  if (pclose(fp) == -1)
    return ERROR;

  if (find > 0)
    return OK;

  system ("mkdir -p /media/usbdisk");
  system ("mount -t vfat /dev/sda1 /media/usbdisk/");

  return OK;
}




/* Comment ********************************************************************
 * 업데이트할 파일 이름을 찾습니다.
 *
 */
static int 
find_file (char *dst, struct stamp_tag *file_tag)
{
  char *ptr;
  int index = 0;

  // 이미지를 2번 체크하기 때문에 주의해야 한다.
  if (!strncmp (dst, UPDATE_FILE_RULE, strlen (UPDATE_FILE_RULE)))
  {
    ptr = strtok (dst, "._- ");
    index++;

    if (strncmp (ptr, UPDATE_OS_RULE, strlen (UPDATE_OS_RULE)))
    {
      file_tag->stamp = 0;
      return ERROR;
    }

    while ((ptr = strtok(NULL, "._- ")) != NULL)
    {
      if (index == 1)
        {
          if (strncmp (ptr, UPDATE_DEVICE_RULE, strlen (UPDATE_DEVICE_RULE)))
          {
            file_tag->stamp = 0;
            return ERROR;
          }
        }
      else if (index == 2)
        {
          file_tag->stamp = atoi(ptr);
        }
      else if (index == 3)
        {
          if (strncmp (ptr, "bin", strlen ("bin")))
          {
            file_tag->stamp = 0;
            return ERROR;
          }
        }

      index++;
    }


    return OK;
  }

  return ERROR;
}




/* Comment ********************************************************************
 * 해당하는 경로의 파일에서 sha1sum 값을 얻옵니다.
 *
 */
static void 
get_sha1sum_from_image (struct stamp_tag *file_tag)
{
  FILE *fp;
  char cmd[BUF_SIZE + 1];
  char buf[BUF_SIZE + 1];

  // 이미지의 경로가 바른지 확인합니다.
  if (0 != access (file_tag->path, R_OK))
  {
    memset (file_tag->sha1sum, 0x00, sizeof(file_tag->sha1sum));
    return;
  }

  memset (cmd, 0x00, sizeof(cmd));
  snprintf (cmd, sizeof (cmd), "/usr/bin/sha1sum %s | awk '{print $1}'", file_tag->path);

  fp = popen(cmd, "r");

  if (!fp)
    return;

  fgets(buf, sizeof(buf), fp);
  buf[strlen(buf) - 1] = '\0';

  if (pclose(fp) == -1)
    return;

  memset (file_tag->sha1sum, 0x00, sizeof(file_tag->sha1sum));
  snprintf(file_tag->sha1sum, sizeof(file_tag->sha1sum), "%s", buf);

  return;
}



/* Comment ********************************************************************
 * 해당하는 경로의 파일의 크기를 얻어옵니다.
 *
 */
static void 
get_filesize_from_image (struct stamp_tag *file_tag)
{
  struct  stat  file_info;

  if ( 0 > stat(file_tag->path, &file_info))
    file_tag->filesize = 0;
  else
    file_tag->filesize = file_info.st_size;

  return;
}



/* Comment ********************************************************************
 * /media/usbdisk 에서 업데이트 할 항목을 찾습니다.
 *
 */
static int 
check_update (struct stamp_tag *file_tag, struct stamp_tag *device_tag)
{
  DIR            *dir_info;
  struct dirent  *dir_entry;
  char copy_name[256];

  // /media/usbdisk가 있는지 확인 합니다.
  if (access_mount_path() == ERROR)
    return ERROR;

  dir_info = opendir(MOUNT_PATH);
  if ( NULL != dir_info)
  {
    while( dir_entry  = readdir( dir_info))
    {
      memset (copy_name, 0x00, sizeof(copy_name));
      sprintf (copy_name, "%s", dir_entry->d_name);

      if (find_file(dir_entry->d_name, file_tag) == OK)
      {
        if (file_tag->stamp > device_tag->stamp)
        {
          // 업데이트할 파일의 경로를 추가합니다.
          sprintf(file_tag->path, "/media/usbdisk/%s", copy_name);
          // 업데이트할 파일의 sha1sum 값을 가지고 옵니다.
          get_sha1sum_from_image (file_tag);
          // 업데이트할 파일의 사이즈를 가지고 옵니다.
          get_filesize_from_image (file_tag);
          return OK;
        }
      }
    }
    closedir( dir_info);
  }

  return ERROR;
}


/* Comment ********************************************************************
 * 이미지 파일을 업데이트 하기 위하여 MTDBLOCK을 지웁니다.
 *
 */
static int
erase_mtdblcok (char *new_path)
{
  char cmd[BUF_SIZE + 1];

  printf ("%s \r\n", new_path);
  if (0 == access (new_path, R_OK))
    {
      memset (cmd, 0x00, sizeof(cmd));
      sprintf (cmd,
              "%s %s",
              FLASH_ERASEALL_BIN,
              DEV_MTD4_PATH);
      printf ("%s \r\n", cmd);
      system (cmd);

      return OK;
    }
  else
    {
      return ERROR;
    }

  return ERROR;
}





/* Comment ********************************************************************
 * 이미지 파일을 업데이트 합니다.
 *
 */
static int
write_image_to_mtdblock (char *new_path)
{
  char cmd[BUF_SIZE + 1];

  printf ("%s \r\n", new_path);
  if (0 == access (new_path, R_OK))
    {
      memset (cmd, 0x00, sizeof(cmd));
      sprintf (cmd,
              "dd if=%s of=%s conv=sync 2> /dev/null",
              new_path,
              DEV_MTD4_PATH);
      printf ("%s \r\n", cmd);
      system (cmd);
      return OK;
    }
  else
    {
      return ERROR;
    }

  return ERROR;
}




/* Comment ********************************************************************
 * sha1sum 값을 비교합니다.
 *
 */
static int
compare_sha1sum (struct stamp_tag *dst, struct stamp_tag *src)
{
  // MTD Block에서 가지고온 이미지의 sha1sum 값과 파일의 sha1sum을 비교합니다.
  if (!strncmp (dst->sha1sum, src->sha1sum, strlen (dst->sha1sum)))
    return OK;

  return ERROR;
}



/* Comment ********************************************************************
 * 업데이트 진행상태를 LED로 표현합니다.
 *
 */
void *status_thread (void *arg)
{
  time_t now_time;
  time_t prev_time;

  time (&now_time);
  time (&prev_time);

  for(;;)
  {
    time (&now_time);

    // 600초(10분) 동안 상태가 완료되지 않으면 에러로 표기합니다.
    if ((now_time - prev_time) > 600 && status_flag != STATUS_COMPLETE)
      status_flag = STATUS_ERROR;

    if (status_flag == STATUS_READY)
    {
      ; // do notting.
    }
    else if (status_flag == STATUS_UPDATE)
    {
      system ("echo pup > /proc/nexg/led/status");
      usleep (100000);
      system ("echo pdn > /proc/nexg/led/status");
      usleep (100000);
    }
    else if (status_flag == STATUS_COMPLETE)
    {
      system ("echo pup > /proc/nexg/led/status");
      system ("echo pup > /proc/nexg/led/lte");
      usleep (400000);
      system ("echo pdn > /proc/nexg/led/status");
      system ("echo pdn > /proc/nexg/led/lte");
      usleep (400000);

    }
    else if (status_flag == STATUS_ERROR)
    {
      system ("echo pup > /proc/nexg/led/status");
      system ("echo pdn > /proc/nexg/led/lte");
      usleep (400000);
      system ("echo pdn > /proc/nexg/led/status");
      system ("echo pup > /proc/nexg/led/lte");
      usleep (400000);
    }
  }
}




/* Comment ********************************************************************
 * USB DISK로 부터 자동으로 업데이트를 진행합니다.
 *
 */
int
update_from_usbdisk (void)
{
  int i = 0;
  struct stamp_tag file;
  struct stamp_tag device;
  struct stamp_tag mtd_image;

  memset(&file, 0x00, sizeof(file));
  memset(&device, 0x00, sizeof(device));
  memset(&mtd_image, 0x00, sizeof(mtd_image));


  // 부팅시 자동으로 업데이트 하기 위하여..
  // 다른 초기화 코드들이  다 실행 될때까지 기다립니다.
  // 넉넉히 10초를 쉬도록 하였습니다.
  //STATUS_PRINT(STATUS_WAIT_10SEC);
  sleep (10);

  // devcie의 정보를 먼저 읽어오도록 합니다.
  if (init_device_info(&device) == ERROR)
  {
    ERR_PRINT(ERR_NOT_FOUND_DEVCIE_VERSION);
    return ERROR;
  }
  //STATUS_PRINT(STATUS_DEVICE_INIT);

  // USB가 올바로 준비되었는지 확인합니다.
  if (init_usbdisk_for_update() == ERROR)
  {
    ERR_PRINT(ERR_NOT_FOUND_USBDISK);
    return ERROR;
  }
  //STATUS_PRINT(STATUS_USBDISK_INIT);

  // USB 를 /media/usbdisk로 마운트 시킵니다.
  if (mount_usbdisk () == ERROR)
  {
    ERR_PRINT(ERR_USBDISK_MOUNT);
    return ERROR;
  }
  //STATUS_PRINT(STATUS_USBDISK_MOUNT);

  // mtdblock 을 지울 실행파일이 있는지 확인합니다.
  if (init_mtdutils() == ERROR)
  {
    ERR_PRINT(ERR_NOT_FOUND_MTDUTILS);
    return ERROR;
  }
  //STATUS_PRINT(STATUS_READY_MTDUTILS);

  // update 파일이 있는지 확인합니다.
  // 파일명으로 검색하여 확인하도록 합니다.
  if (check_update(&file, &device) == ERROR)
  {
    // 이 경우에는 업데이틀 할 필요가 없기 때문에 프로그램을 종료합니다.
    //NOTICE_PRINT(NOTI_LATEST_BOOTLOADER);
    status_flag = STATUS_READY;
    sleep (1);
    system ("echo pup > /proc/nexg/led/status");
    system ("echo pdn > /proc/nexg/led/lte");
    system ("sync");
    system ("sync");
    system ("sync");
    exit (0);
  }

  // 장비의 버전과 업데이트 하고자 하는 버번이 같은 경우.
  if (file.stamp == device.stamp)
  {
    // 이 경우에는 업데이틀 할 필요가 없기 때문에 프로그램을 종료합니다.
    NOTICE_PRINT(NOTI_LATEST_BOOTLOADER);
    status_flag = STATUS_COMPLETE;

    system ("sync");
    system ("sync");
    system ("sync");
    for(;;){sleep(60);}
  }

  status_flag = STATUS_UPDATE;
  printf("\n\n\n\n");
  STATUS_PRINT(STATUS_START);

  // 최대 3번까지 재시도를 합니다.
  // 이후에는 장비 에러로 표기합니다.
  for (i = 0; i < MAX_UPDATE_COUNT; i++)
  {
    // 이미지 업데이트를 진행합니다.
    STATUS_PRINT(STATUS_ERASE_MTDBLOCK);
    STATUS_PRINT(STATUS_WAIT_5MIN);
    erase_mtdblcok(file.path);

    STATUS_PRINT(STATUS_WRITE_IMAGE);
    write_image_to_mtdblock(file.path);

    // mtdblock 의 커널을 읽어오도록 합니다.
    // mtdblock 4의 모든 내용을 /tmp/image.bin 으로 복사합니다.
    // 그리고 위에서 읽어온 파일의 사이즈 만큼 잘라냅니다.
    if (copy_mtdblock_image(&file) == ERROR)
    {
      ERR_PRINT(ERR_NOT_FOUND_MTDBLOCK);
      continue;
    }
    STATUS_PRINT(STATUS_MTDBLOCK_COPY);

    // MTD Block 에서 가지고 이미지의 경로를 설정합니다.
    memset (mtd_image.path, 0x00, sizeof(mtd_image.path));
    sprintf (mtd_image.path, "%s", TMP_SPLIT_IMAGE_PATH);

    // SHA1SUM 값을 가지고 옵니다.
    get_sha1sum_from_image(&mtd_image);

    // 복사된 이미지를 비교합니다.
    // 앞선 코드에서 잘라진 파일의 sha1sum을 서로 비교하게 됩니다.
    if (compare_sha1sum (&mtd_image, &file) == ERROR)
    {
      ERR_PRINT(ERR_COPY_IMAGE);
      continue;
    }
    STATUS_PRINT(STATUS_UPDATE_COMPLETE);
    return OK;
  }

  return ERROR;
}



/* Comment ********************************************************************
 * 주어진 파일명의 이미지로 장비를 업데이트 합니다.
 *
 */
static int
update_from_file(char *src)
{
  int i = 0;
  struct stamp_tag file;
  struct stamp_tag mtd_image;

  memset(&file, 0x00, sizeof(file));
  memset(&mtd_image, 0x00, sizeof(mtd_image));

  printf("\n\n\n\n");
  STATUS_PRINT(STATUS_START);

  if (init_file_path(src, &file) == ERROR)
  {
    ERR_PRINT(ERR_NOT_FOUND_FILE);
    return ERROR;
  }
  STATUS_PRINT(STATUS_READY_FILE);

  // 반드시 실행되어야 하는 작업니다.
  // 업데이트할 파일의 SHA1SUM 값을 가지고 옵니다.
  get_sha1sum_from_image(&file);

  // 반드시 실행되어야 하는 작업니다.
  // 업데이트할 파일의 사이즈를 가지고 옵니다.
  get_filesize_from_image (&file);

  // mtdblock 을 지울 실행파일이 있는지 확인합니다.
  if (init_mtdutils() == ERROR)
  {
    ERR_PRINT(ERR_NOT_FOUND_MTDUTILS);
    return ERROR;
  }
  STATUS_PRINT(STATUS_READY_MTDUTILS);

  // 최대 3번까지 재시도를 합니다.
  // 이후에는 장비 에러로 표기합니다.
  for (i = 0; i < MAX_UPDATE_COUNT; i++)
  {
    // 이미지 업데이트를 진행합니다.
    STATUS_PRINT(STATUS_ERASE_MTDBLOCK);
    STATUS_PRINT(STATUS_WAIT_5MIN);
    erase_mtdblcok(file.path);

    STATUS_PRINT(STATUS_WRITE_IMAGE);
    write_image_to_mtdblock(file.path);

    // mtdblock 의 커널을 읽어오도록 합니다.
    // mtdblock 4의 모든 내용을 /tmp/image.bin 으로 복사합니다.
    // 그리고 위에서 읽어온 파일의 사이즈 만큼 잘라냅니다.
    if (copy_mtdblock_image(&file) == ERROR)
    {
      ERR_PRINT(ERR_NOT_FOUND_MTDBLOCK);
      continue;
    }
    STATUS_PRINT(STATUS_MTDBLOCK_COPY);

    // MTD Block 에서 가지고 이미지의 경로를 설정합니다.
    memset (mtd_image.path, 0x00, sizeof(mtd_image.path));
    sprintf (mtd_image.path, "%s", TMP_SPLIT_IMAGE_PATH);

    // SHA1SUM 값을 가지고 옵니다.
    get_sha1sum_from_image(&mtd_image);

    // 복사된 이미지를 비교합니다.
    // 앞선 코드에서 잘라진 파일의 sha1sum을 서로 비교하게 됩니다.
    if (compare_sha1sum (&mtd_image, &file) == ERROR)
    {
      ERR_PRINT(ERR_COPY_IMAGE);
      continue;
    }
    STATUS_PRINT(STATUS_UPDATE_COMPLETE);
    return OK;
  }

  return ERROR;
}



/* Comment ********************************************************************
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  int thr_id;
  int dummy;
  int ret = 0;
  pthread_t p_thread;

  if (argc != 3)
  {
    printf("\n\nWrong command\n");
    printf(" - boot system [image name]\n");
    printf(" - boot auto usbdisk\n");
    return 0;
  }

  if (!strncmp ("system", argv[1], strlen ("system")))
  {
    thr_id = pthread_create(&p_thread, NULL, status_thread, (void *)dummy);
    if (thr_id < 0)
    {
      perror("thread create error : ");
      exit(0);
    }
    pthread_detach(p_thread);

    ret = update_from_file(argv[2]);

    pthread_cancel(p_thread);
  }
  else if (!strncmp ("auto", argv[1], strlen ("auto")))
  {
    if (!strncmp ("usbdisk", argv[2], strlen ("usbdisk")) )
    {
      thr_id = pthread_create(&p_thread, NULL, status_thread, (void *)dummy);
      if (thr_id < 0)
      {
        perror("thread create error : ");
        exit(0);
      }
      pthread_detach(p_thread);



      ret = update_from_usbdisk();

      // 업데이트 결과에 따라 상태값을 변경합니다.
      if (ret == OK)
        status_flag = STATUS_COMPLETE;
      else
        status_flag = STATUS_ERROR;

      // 자동으로 연결하는 경우에는 사용자가 콘솔을 연결하지 않기 때문에..
      // 사용자가 결과를 외부에서 확인할 수 있도록 하기 위하여
      // 사용자가 종료할 때 까지 프로그램을 종료하지 않습니다.
      system ("sync");
      system ("sync");
      system ("sync");
      for(;;){sleep(60);}
    }
  }

  system ("echo pup > /proc/nexg/led/status");

  return 0;
}


