#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

void get_tm400_status (ZEBOS_LTE_MODULE_t *mod);
int set_tm400_ota (int type);
int set_tm400_apn (int type, char *apn_url);
void read_tm400_apn (lte_apn_info_t *apn_info);
int read_tm400_network_status (void);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
