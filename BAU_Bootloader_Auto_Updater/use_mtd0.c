#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "use_mtd0.h"

#define BUF_SIZE                        16
#define STRING_LENGTH                   128
#define STAMP_LENGTH                    32

#define OK                              0
#define ERROR                           -1

#define FAILSAFE_UBOOT                  0
#define NORMAL_UBOOT                    1
#define MAX_UBOOT_TAG                   4
#define FIND_UBOOT_TAG                  1

// 12byte의 tag 메시지는 항상 동일하다.
char uboot_tag[] =
{
  0x10, 0x00, 0x01, 0x3F, 0x00,
  0x00, 0x00, 0x00, 0x42, 0x4F,
  0x4F, 0x54
};

struct model_list
{
  int board_type;
  char bootloader_name[32];
};

struct dev_mem_info
{
  int board;
  int size_0;
  int block_0;
  int size_1;
  int block_1;
};

struct dev_mem_info mem_list[] =
{
  {20500, 0x2000, 8, 0x10000, 63},
};

struct find_uboot_info
{
  int tag;
  unsigned int addr;
};


/* Comment ********************************************************************
 * uboot의 tag가 존재하는 sector를 찾는다.
 *
 */
static int
find_flash_sector (unsigned int dest_addr)
{
  int i = 0;
  int sector = 0;
  unsigned int addr = 0;

  sector = 0;
  for (i = 0; i < mem_list[0].block_0; i++)
    {
      if (addr == dest_addr)
        return sector;

      sector++;
      addr += mem_list[0].size_0;
    }

  for (i = 0; i < mem_list[0].block_1; i++)
    {
      if (addr == dest_addr)
        return sector;

      sector++;
      addr += mem_list[0].size_1;
    }

  return -1;
}



/* Comment ********************************************************************
 * 파일의 길이를 리턴한다.
 *
 */
static int
get_file_size (char *pfile)
{
  int fd = 0;
  int len = 0;

  fd = open (pfile, O_RDONLY);

  len = lseek (fd, 0L, SEEK_END);

  close (fd);

  return len;
}


/* Comment ********************************************************************
 * uboot 파일을 16byte씩 읽어오도록 한다.
 *
 */
static int
find_tag (char *pfile, int max_len, struct find_uboot_info *plist)
{
  FILE *fp = NULL;
  char buf [BUF_SIZE + 1];
  int sector = 0;
  unsigned int count = 0;
  int idx = 0;

  fp = fopen (pfile, "r");

  count = 0;
  memset (buf, 0x00, BUF_SIZE);
  idx = 0;

  while (!feof(fp))
    {
      fread(buf, 1, 16, fp);
      if (buf[0] == uboot_tag[0] && buf[3] == uboot_tag[3])
        {
          if (buf[8] == uboot_tag[8] && buf[9] == uboot_tag[9])
            {
              if (strncmp (buf, uboot_tag, 16) == 0)
                {
                  sector = find_flash_sector (count*16);
                  printf ("FIND: sector %d (0x%x) \r\n", sector, count*16);
                  plist[idx].tag = FIND_UBOOT_TAG;
                  plist[idx].addr = count*16;
                  if (idx++ > NORMAL_UBOOT)
                    break;
                }
            }
        }
      count++;
      memset (buf, 0x00, BUF_SIZE);
    }

  fclose (fp);

  return 0;
}


/* Comment ********************************************************************
 * NORMAL_UBOOT 영역을 삭제합니다.
 *
 */
static void
remove_uboot (struct find_uboot_info *plist)
{
  char cmd [BUF_SIZE + 1];

  memset (cmd, 0x00, sizeof(cmd));

  sprintf (cmd, "flash_erase /dev/mtd0 0x%x 15", plist->addr);
  printf ("REMOVE: addr = 0x%x \r\n", plist->addr);
  printf ("SYSTEM: %s \r\n", cmd);
  //system (cmd);
}



/* Comment ********************************************************************
 * main function.
 *
 */
//int main(int argc, char *argv[])
int api_flash(int argc, char *argv[])
{
  int max_len = 0;
  struct find_uboot_info uboot_list[MAX_UBOOT_TAG];

  memset (uboot_list, 0x00, sizeof(uboot_list));

  system ("dd if=/dev/mtd0 of=/tmp/mtd0.bin conv=sync 2> /dev/null");

  max_len = get_file_size ("/tmp/mtd0.bin");
  find_tag ("/tmp/mtd0.bin", max_len, uboot_list);

  if (uboot_list[NORMAL_UBOOT].tag == FIND_UBOOT_TAG)
    remove_uboot(&uboot_list[NORMAL_UBOOT]);

  return 0;
}


