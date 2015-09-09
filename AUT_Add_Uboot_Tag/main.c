#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE                        16
#define STRING_LENGTH                   128
#define STAMP_LENGTH                    32

#define OK                              0
#define ERROR                           -1

#define FAILSAFE_UBOOT                  0
#define NORMAL_UBOOT                    1
#define MAX_UBOOT_TAG                   4
#define FIND_UBOOT_TAG                  1

#define UINT                            unsigned int

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

struct model_list boot_list[] =
{
  {20500, "u-boot-octeon_vf0500.bin"},
  {21500, "u-boot-octeon_vf1500.bin"},
  {22501, "u-boot-octeon_vf2501.bin"},
  {23500, "u-boot-octeon_vf3500.bin"},
  {-1, "unknown"},
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
 * 파일의 길이를 리턴한다.
 *
 */
static UINT
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
 *
 *
 */static UINT
get_tag_location (UINT length)
{
  UINT offset = 0;

  offset = length / 0x10;
  printf ("#J: offset = %d (0x%x)\r\n", offset , offset );
  while ((length - offset) >= 0 )
    {
      offset += 0x10;
      //printf ("#J: offset = %d (0x%x)\r\n", offset , offset );
    }

  return offset;  
}

/* Comment ********************************************************************
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  UINT uboot_length = 0;
  UINT tag_loc = 0;

  uboot_length = get_file_size ("./uboot.bin");
  printf ("#J: length = %d (0x%x) \r\n", uboot_length, uboot_length);

  if (uboot_length > 0)
    {
      tag_loc = get_tag_location (uboot_length);
      printf ("#J: tag_loc = %d  (0x%x)\r\n", tag_loc, tag_loc);
    }

  return 0;
}


