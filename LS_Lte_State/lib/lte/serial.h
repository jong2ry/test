#include "pal.h"

#ifdef HAVE_LTE_INTERFACE

//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
//#define ltedebug    console_print
#define ltedebug(x...)

//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
int lte_strtok (char *pbuf, int buf_size, int index, char *pret);
int lte_strtok_by_user (char *pbuf, int buf_size, int index, char *pret, char *ptok);
int lte_lookup_string_from_buf (char *pSrc, int length, char *pDst);
int lte_xmit_sms (char *pMsgCmd, char *pRetStr, int wait);
int lte_xmit_serial (char *pMsgCmd, char *pRetStr);

#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */

