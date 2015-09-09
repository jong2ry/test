#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

void get_lgm2m_pegasus_status (ZEBOS_LTE_MODULE_t *mod);
//void get_lgm2m_pegasus__detail_status (struct cli *cli);
int sendmsg_lgm2m_pegasus (struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg);
//int set_lgm2m_pegasus_ota (int type);
int set_lgm2m_pegasus_apn (int type, char *apn_url);
void read_lgm2m_pegasus_apn (lte_apn_info_t *apn_info);
int read_lgm2m_pegasus_network_status (void);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
