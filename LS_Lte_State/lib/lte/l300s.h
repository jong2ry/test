#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

void get_l300s_status (ZEBOS_LTE_MODULE_t *mod);
//void get_l300s_detail_status (struct cli *cli);
int sendmsg_l300s (struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg);
int set_l300s_ota (int type);
int set_l300s_apn (int type, char *apn_url);
void read_l300s_apn (lte_apn_info_t *apn_info);
int read_l300s_network_status (void);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
