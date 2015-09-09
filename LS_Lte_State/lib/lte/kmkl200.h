#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

void get_kmkl200_status (ZEBOS_LTE_MODULE_t *mod);
void get_kmkl200_detail_status (struct cli *cli);
int sendmsg_kmkl200 (struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg);
int set_kmkl200_ota (int type);
int set_kmkl200_apn (int type, char *apn_url);
void read_kmkl200_apn (lte_apn_info_t *apn_info);
int read_kmkl200_network_status  (void);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
