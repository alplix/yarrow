/* Yarrow - FNIRSI GC-03 memory map.
 * The stock bootloader owns 0x08000000-0x08005FFF and jumps to the application at 0x08006000.
 * The last 1 KB page (0x0803FC00) holds factory tube calibration: it must never be erased or written. */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08006000, LENGTH = 96K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 31K
}

ENTRY(reset_handler)

SECTIONS
{
  .text :
  {
    KEEP(*(.vectors))
    . = ORIGIN(FLASH) + 0x200;     /* room for the full interrupt vector table */
    KEEP(*(.header))               /* image header at 0x200, see image.h */
    KEEP(*(.boot))                 /* integrity check and fault handlers, first in flash */
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .bss (NOLOAD) :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  _image_end = LOADADDR(.data) + SIZEOF(.data);
  _stack_top = ORIGIN(RAM) + LENGTH(RAM);

  /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
