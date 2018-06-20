#include "kernel.h"
#include "input.h"

//***********************************************
//KEYBOARD FUNCTIONS
//***********************************************

unsigned char kb_read_scan_code(void)
{
    return inb(0x60);
}

char input_get_key(unsigned char scancode)
{
    char keymap[100] = "`1234567890-=  qwertyuiop[]  asdfghjkl;'| <zxcvbnm,./ ";

    if(scancode <= 0x58)
    {
        return keymap[scancode - 1];
    }
    else
    {
        return 0;
    }
}
