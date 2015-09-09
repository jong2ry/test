#define GIT_TAG                         "GIT"
#define STAMP_TAG                       "T"
#define END_TAG                         "AA"

#define DEV_MTD1_PATH                   "/dev/mtd1"
#define SYS_MTD1_NAME_PATH              "/sys/devices/virtual/mtd/mtd1/name"
#define TMP_UBOOT_PATH                  "/tmp/uboot.bin"
#define UPDATE_SERVER_PATH              "ftp://jong2ry:nexg@10.200.0.200:/"
#define FLASH_ERASEALL_BIN              "/usr/sbin/flash_eraseall"

#define ERROR_LINE         "*********************************************\r\n"
#define ERR_NOT_FOUND_MTDBLOCK          "Can't find mtdblock1 for u-boot"
#define ERR_NOT_FOUND_MTDUTILS          "Can't find mtdutils"
#define ERR_BOOTLOADER_DOWNLOAD_FAILED  "Bootloader download failed"
#define ERR_NOT_FOUND_TAG               "Can't find the bootloader tag of download image"
#define ERR_NOT_FOUND_BOOTLOADER_PATH   "Can't find bootloader path"
#define ERR_PRINT(x) {printf("\r\n%s %%ERR : %s \r\n\r\n", ERROR_LINE, x);}

#define LINE               "#############################################\r\n"
#define NOTI_NOT_FOUND_TAG             "Can't find the bootloader tag of device"
#define NOTI_LATEST_BOOTLOADER         "Your bootloader is up to date."
//#define NOTICE_PRINT(x) {printf("\r\n%s#  NOTI : %s \r\n%s\r\n", LINE, x, LINE);}
#define NOTICE_PRINT(x) {printf("\r\n# NOTI : %s \r\n\r\n", x);}

#define STATE_PRINT(x) {printf("BAU : %s \r\n", x);}

