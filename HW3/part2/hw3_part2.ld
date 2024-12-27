ENTRY(_hw3_unicorn)

EXTERN(purple white)

PHDRS
{
    text PT_LOAD FLAGS(3);           /* write + execute */
    data PT_LOAD FLAGS(6);           /* read + write */
    rodata PT_LOAD FLAGS(5);         /* read + execute */
}


SECTIONS
{
  . = 0x400000;
  .text :
  {
    *(.text)
  } : text
  
  . = 0x60000;
  .data : 
  {
	  *(.data)
	  *(.bss)
  } : data

  . = 0x80000000;
  .rodata :
  {
    *(.rodata)
  } : rodata 
}
