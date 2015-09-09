/* 리눅스에서 적용되어 있는 하드웨어 관련 코드입니다. 
 * Zebos 및 다른 App에서 사용하기 편하도록 공통파일을 생성하여
 * stagin_dir/include 에 링크를 걸어서 사용하도록 합니다. 
 * GPIO 및 IOCTL 관련 내용이 포함되어 있습니다. 
 *
 */
#ifndef __NEXG_COMMON_DRV
#define __NEXG_COMMON_DRV


//----------------------------------------------------------------------------
// GPIO DEFINITION
//----------------------------------------------------------------------------
#define GPIO_OUTPUT         1
#define GPIO_INPUT          0

typedef struct  _gpio_list 
{
  int num;
  int direction;
  char *name;
} GPIO_LIST;


//----------------------------------------------------------------------------
// VF403 GPIO LIST
//----------------------------------------------------------------------------
#define VF403_LED_GPIO          6
#define VF403_SYSLTE_GPIO       3


//----------------------------------------------------------------------------
// VF500 GPIO LIST
//----------------------------------------------------------------------------
#define VF500_PCIE_H_PWR_GPIO   10


//----------------------------------------------------------------------------
// x500 GPIO LIST (KTNF)
//----------------------------------------------------------------------------
#define VF3500_LED_GPIO         1
#define VF3408N_LED_GPIO        1
#define VF4500_LED_GPIO         1
#define VF5500_LED_GPIO         1
#define VF5408N_LED_GPIO        1


//----------------------------------------------------------------------------
// VF50 GPIO LIST
//----------------------------------------------------------------------------
#define VF50_SMI_SCK            36
#define VF50_SMI_SDA            37
#define VF50_FACTORYRESET       38
#define VF50_SIG_LED            40
#define VF50_STATUS_LED         44
#define VF50_LTE_LED            45
#define VF50_PCIE_H_RESET       46
#define VF50_PCIE_H_PWR         47
#define VF50_PCIE_L_RESET       48
#define VF50_PCIE_L_PWR         49
#define VF50_SYSLTE_RESET       VF50_PCIE_H_RESET
#define VF50_SYSLTE_PWR         VF50_PCIE_H_PWR
#define VF50_SYSWLAN_RESET      VF50_PCIE_L_RESET
#define VF50_SYSWLAN_PWR        VF50_PCIE_L_PWR


//----------------------------------------------------------------------------
// LS_LTER GPIO LIST 
//----------------------------------------------------------------------------
#define LS_LTER_SIG_LED           37
#define LS_LTER_CON_IN            38   /* IN */
#define LS_LTER_DIS_IN            39   /* IN */
#define LS_LTER_OUT_ALM           40
#define LS_LTER_OUT_ST1           41
#define LS_LTER_OUT_ST2           42
#define LS_LTER_OUT_ST3           43
#define LS_LTER_STATUS_LED        44
#define LS_LTER_CON_LED           45
#define LS_LTER_ERR_LED           46
#define LS_LTER_AUX_IN            47   /* IN */  // LS_LTER_FAC_IN과 동일합니다.
#define LS_LTER_FAC_IN            47   /* IN */  // LS_LTER_AUX_IN과 동일합니다.
#define LS_LTER_PCIE_RESET        48
#define LS_LTER_PCIE_PWR          49
#define LS_LTER_SYSLTE_RESET      LS_LTER_PCIE_RESET
#define LS_LTER_SYSLTE_PWR        LS_LTER_PCIE_PWR


//----------------------------------------------------------------------------
// VF50L GPIO LIST 
//----------------------------------------------------------------------------
#define VF50L_SIG_LED             37
#define VF50L_STATUS_LED          44
#define VF50L_PCIE_RESET          48
#define VF50L_PCIE_PWR            49
#define VF50L_SYSLTE_RESET        VF50L_PCIE_RESET
#define VF50L_SYSLTE_PWR          VF50L_PCIE_PWR


//----------------------------------------------------------------------------
// M2MG20 GPIO LIST
//----------------------------------------------------------------------------
#define M2MG20_SMI_SCK            36
#define M2MG20_SMI_SDA            37
#define M2MG20_FACTORYRESET       38
#define M2MG20_SIG_LED            40
#define M2MG20_STATUS_LED         44
#define M2MG20_LTE_LED            45
#define M2MG20_PCIE_H_RESET       46
#define M2MG20_PCIE_H_PWR         47
#define M2MG20_PCIE_L_RESET       48
#define M2MG20_PCIE_L_PWR         49
#define M2MG20_SYSLTE_RESET       M2MG20_PCIE_H_RESET
#define M2MG20_SYSLTE_PWR         M2MG20_PCIE_H_PWR
#define M2MG20_SYSWLAN_RESET      M2MG20_PCIE_L_RESET
#define M2MG20_SYSWLAN_PWR        M2MG20_PCIE_L_PWR


//----------------------------------------------------------------------------
// M2MG20L GPIO LIST 
//----------------------------------------------------------------------------
#define M2MG20L_SIG_LED             37
#define M2MG20L_FAC_IN              38
#define M2MG20L_485EN               39
#define M2MG20L_STATUS_LED          44
#define M2MG20L_LTE_LED             45
#define M2MG20L_SYSLTE_RESET        46
#define M2MG20L_SYSLTE_PWR          47
#define M2MG20L_PCIE_RESET          48
#define M2MG20L_PCIE_PWR            49


//----------------------------------------------------------------------------
// MINOR NUMBER LIST
//----------------------------------------------------------------------------
#define NEXG_SYSLED_MINOR         246
#define NEXG_PCIE0_MINOR          247
#define NEXG_PCIE1_MINOR          248
#define NEXG_SYSLTE_MINOR         249
#define NEXG_SYSM2M_MINOR         250


//----------------------------------------------------------------------------
// SYSM2M IOCTL DEFINITION
//----------------------------------------------------------------------------
#define SYSM2M_IOCTL_MAGIC        'M'
#define SYSM2M_IOCTL_DI_READ      _IOR(SYSM2M_IOCTL_MAGIC, 0, int)
#define SYSM2M_IOCTL_FAC_READ     _IOR(SYSM2M_IOCTL_MAGIC, 1, int)
#define SYSM2M_IOCTL_READY        _IO(SYSM2M_IOCTL_MAGIC, 2)
#define SYSM2M_IOCTL_CONNECT      _IO(SYSM2M_IOCTL_MAGIC, 3)
#define SYSM2M_IOCTL_DISCONNECT   _IO(SYSM2M_IOCTL_MAGIC, 4)
#define SYSM2M_IOCTL_NO_USIM      _IO(SYSM2M_IOCTL_MAGIC, 5)
#define SYSM2M_IOCTL_NO_SIGNAL    _IO(SYSM2M_IOCTL_MAGIC, 6)
#define SYSM2M_IOCTL_ALRAM        _IO(SYSM2M_IOCTL_MAGIC, 7)
#define SYSM2M_IOCTL_MAX          15   // 인덱스의 최대 갯수

// DI signal's state value
#define M2M_DI_CONNECT            0
#define M2M_DI_DISCONNECT         1
// Factory-Reset button's state value
#define M2M_FAC_NONE              2
#define M2M_FAC_RESET             3


//----------------------------------------------------------------------------
// SYSPCIE IOCTL DEFINITION
//----------------------------------------------------------------------------
#define SYSPCIE_IOCTL_MAGIC       'P'
#define SYSPCIE_IOCTL_ON          _IO(SYSPCIE_IOCTL_MAGIC, 0)
#define SYSPCIE_IOCTL_OFF         _IO(SYSPCIE_IOCTL_MAGIC, 1)
#define SYSPCIE0_IOCTL_PUP        _IO(SYSPCIE_IOCTL_MAGIC, 2)
#define SYSPCIE0_IOCTL_PDN        _IO(SYSPCIE_IOCTL_MAGIC, 3)
#define SYSPCIE0_IOCTL_RESET      _IO(SYSPCIE_IOCTL_MAGIC, 4)
#define SYSPCIE1_IOCTL_PUP        _IO(SYSPCIE_IOCTL_MAGIC, 5)
#define SYSPCIE1_IOCTL_PDN        _IO(SYSPCIE_IOCTL_MAGIC, 6)
#define SYSPCIE1_IOCTL_RESET      _IO(SYSPCIE_IOCTL_MAGIC, 7)
#define SYSLTE_IOCTL_PUP          _IO(SYSPCIE_IOCTL_MAGIC, 8)
#define SYSLTE_IOCTL_PDN          _IO(SYSPCIE_IOCTL_MAGIC, 9)
#define SYSLTE_IOCTL_RESET        _IO(SYSPCIE_IOCTL_MAGIC, 10)
#define SYSPCIE_IOCTL_MAX         15   // 인덱스의 최대 갯수

//----------------------------------------------------------------------------
// SYSLED IOCTL DEFINITION
//----------------------------------------------------------------------------
#define SYSLED_IOCTL_MAGIC        'L'
#define SYSLED_IOCTL_ST_PUP       _IO(SYSLED_IOCTL_MAGIC, 0)
#define SYSLED_IOCTL_ST_PDN       _IO(SYSLED_IOCTL_MAGIC, 1)
#define SYSLED_IOCTL_ALM_PUP      _IO(SYSLED_IOCTL_MAGIC, 2)
#define SYSLED_IOCTL_ALM_PDN      _IO(SYSLED_IOCTL_MAGIC, 3)
#define SYSLED_IOCTL_CON_PUP      _IO(SYSLED_IOCTL_MAGIC, 4)
#define SYSLED_IOCTL_CON_PDN      _IO(SYSLED_IOCTL_MAGIC, 5)
#define SYSLED_IOCTL_SIG_PUP      _IO(SYSLED_IOCTL_MAGIC, 6)
#define SYSLED_IOCTL_SIG_PDN      _IO(SYSLED_IOCTL_MAGIC, 7)
#define SYSLED_IOCTL_LTE_PUP      _IO(SYSLED_IOCTL_MAGIC, 8)
#define SYSLED_IOCTL_LTE_PDN      _IO(SYSLED_IOCTL_MAGIC, 9)
#define SYSLED_IOCTL_MAX          15   // 인덱스의 최대 갯수


#endif /* __NEXG_COMMON_DRV */
