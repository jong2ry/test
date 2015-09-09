struct stamp_tag
{
  char ver[12];
  char stamp[12];
};

struct model_list
{
  int board_type;
  char bootloader_name[32];
};

struct model_list boot_list[] =
{
  {20500, "u-boot-octeon_vf0500.bin"},
  {21500, "u-boot-octeon_vf1500.bin"},
  {22501, "u-boot-octeon_vf2501.bin"},
  {23500, "u-boot-octeon_vf3500.bin"},
  {-1, "unknown"},
};

