#ifndef _PAL_H
#define _PAL_H
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

#define pal_strncmp strncmp 
#define pal_snprintf snprintf
#define pal_atoi atoi


// 빌드 오류를 막기 위하여 임시로 선언합니다.
#define NEXG_MODEL_VF403  403

struct cli
{
  int dummy;
};



void cli_out(struct cli *cli, const char *fmt, ...);
int is_nexg_model (int model);

#endif /*_PAL_H */

