#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

void get_ntlm200q_status (ZEBOS_LTE_MODULE_t *mod);
void get_ntlm200q_detail_status (struct cli *cli);
int sendmsg_ntlm200q (struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg);
int set_ntlm200q_ota (int type);
int set_ntlm200q_apn (int type, char *apn_url);
void read_ntlm200q_apn (lte_apn_info_t *apn_info);
int read_ntlm200q_network_status  (void);
int enable_ntlm200q_rmnet (void);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
