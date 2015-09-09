#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

void get_lgm2m_type1_status (ZEBOS_LTE_MODULE_t *mod);
int set_lgm2m_type1_apn (int type, char *apn_url);
void read_lgm2m_type1_apn (lte_apn_info_t *apn_info);
int read_lgm2m_type1_network_status (void);
int is_lgm2m_type1_m2mpcm(void);
#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
