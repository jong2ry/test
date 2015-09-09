#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

void get_mec_status (ZEBOS_LTE_MODULE_t *mod);
int sendmsg_mec(struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
