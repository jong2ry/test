#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

// 다른 모듈과 다르게 소켓으로 통신하기 때문에,
// 소켓 통신이 가능한지를 확인해야 합니다.
int is_ready_lgm2m_type2 (void);

void get_lgm2m_type2_status (ZEBOS_LTE_MODULE_t *mod);
//void get_lgm2m_type2__detail_status (struct cli *cli);
int sendmsg_lgm2m_type2 (struct cli *cli, ZEBOS_LTE_MODULE_t *mod, char *pNumber, char *pMsg);
//int set_lgm2m_type2_ota (int type);
int set_lgm2m_type2_apn (int type, char *apn_url);
void read_lgm2m_type2_apn (lte_apn_info_t *apn_info);
int read_lgm2m_type2_network_status (void);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
