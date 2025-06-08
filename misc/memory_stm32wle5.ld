/* Memories definition */
MEMORY
{
  ROM    (rx)     : ORIGIN = 0x08000000, LENGTH = 0x4000 /* Flash: Bootloader size, say 16KB for now */
  APPROM_A (rx)   : ORIGIN = 0x08004000, LENGTH = 118K   /* Flash: Application slot A  */
  APPROM_B (rx)   : ORIGIN = 0x08021800, LENGTH = 118K   /* Flash: Application slot B  */
  BL_CFG (rx)     : ORIGIN = 0x0803F000, LENGTH = 2K     /* Flash: Bootloader config */
  CFG (rx)        : ORIGIN = 0x0803F800, LENGTH = 2K     /* Flash: Application config */
  RAM1   (xrw)    : ORIGIN = 0x20000000, LENGTH = 32K
  RAM2   (xrw)    : ORIGIN = 0x20008000, LENGTH = 32K
}

__on9_app_a_start__ = ORIGIN(APPROM_A);
__on9_app_a_size__  = LENGTH(APPROM_A);
__on9_app_b_start__ = ORIGIN(APPROM_B);
__on9_app_b_size__  = LENGTH(APPROM_B);
__on9_blcfg_start__ = ORIGIN(BL_CFG);
__on9_blcfg_size__  = LENGTH(BL_CFG);