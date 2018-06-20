OUTPUT_FORMAT(elf32-i386)
ENTRY(loader)                /* the name of the entry label */

SECTIONS {
    . = 0x00200000;          /* the code should be loaded at 2 MB */

    .__mbHeader : AT ( ADDR( .__mbHeader ) ) {
        mboot = .;
        code = .;
        *(.__mbHeader)
    }

    .text ALIGN (0x1000) :  /* align at 4 KB */
    {
        /*code = .;*/
        *(.text)
        *(.rodata)
    }

    .data ALIGN (0x1000) : /* align at 4 KB */
    {
        data = .;
        *(.data)             /* all data sections from all files */
    }

    .bss ALIGN (0x1000) : /* align at 4 KB */
    {
        bss = .;
        /**(COMMON)   */         /* all COMMON sections from all files */
        *(.bss)              /* all bss sections from all files */
    }
}
