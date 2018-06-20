#include "kernel.h"
#include "vbe.h"
#include "input.h"
#include "gui.h"



/*0       | flags             |    (required)
             +-------------------+
     4       | mem_lower         |    (present if flags[0] is set)
     8       | mem_upper         |    (present if flags[0] is set)
             +-------------------+
     12      | boot_device       |    (present if flags[1] is set)
             +-------------------+
     16      | cmdline           |    (present if flags[2] is set)
             +-------------------+
     20      | mods_count        |    (present if flags[3] is set)
     24      | mods_addr         |    (present if flags[3] is set)
             +-------------------+
     28 - 40 | syms              |    (present if flags[4] or
             |                   |                flags[5] is set)
             +-------------------+
     44      | mmap_length       |    (present if flags[6] is set)
     48      | mmap_addr         |    (present if flags[6] is set)
             +-------------------+
     52      | drives_length     |    (present if flags[7] is set)
     56      | drives_addr       |    (present if flags[7] is set)
             +-------------------+
     60      | config_table      |    (present if flags[8] is set)
             +-------------------+
     64      | boot_loader_name  |    (present if flags[9] is set)
             +-------------------+
     68      | apm_table         |    (present if flags[10] is set)
             +-------------------+
     72      | vbe_control_info  |    (present if flags[11] is set)
     76      | vbe_mode_info     |
     80      | vbe_mode          |
     82      | vbe_interface_seg |
     84      | vbe_interface_off |
     86      | vbe_interface_len |
             +-------------------+
     88      | framebuffer_addr  |    (present if flags[12] is set)
     96      | framebuffer_pitch |
     100     | framebuffer_width |
     104     | framebuffer_height|
     108     | framebuffer_bpp   |
     109     | framebuffer_type  |
     110-115 | color_info        |
             +-------------------+*/




//***********************************************
//C FUNCTIONS
//***********************************************

int abs(int num)
{
    if(num >= 0)
    {
        return num;
    }
    else
    {
        return -num;
    }
}








struct gdt {
    unsigned int address;
    unsigned short size;
} __attribute__((packed));

struct SysGDTEntry {
    unsigned int base;
    unsigned int limit;
    unsigned char type;
} __attribute__((packed));


struct cpu_state {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    unsigned int esp;
} __attribute__((packed));

struct stack_state {
    unsigned int error_code;
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
} __attribute__((packed));



int sys_encode_gdt_entry(unsigned char *target, struct SysGDTEntry source)
{
    // Check the limit to make sure that it can be encoded
    if ((source.limit > 65536) && (source.limit & 0xFFF) != 0xFFF)
    {
        return -1;
    }
    if (source.limit > 65536)
    {
        // Adjust granularity if required
        source.limit = source.limit >> 12;
        target[6] = 0xC0;
    }
    else
    {
        target[6] = 0x40;
    }

    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] |= (source.limit >> 16) & 0xF;

    // Encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;

    // And... Type
    target[5] = source.type;

    return 0;
}

void sys_init_gdt(void)
{
    char gdt_size = 3;
    struct SysGDTEntry GDT[gdt_size];

    GDT[0].base = 0;                     // Selector 0x00 cannot be used
    GDT[0].limit = 0;
    GDT[0].type = 0;

    GDT[1].base = 0x0;                     // Selector 0x08 will be our code
    GDT[1].limit = 0xffffffff;
    GDT[1].type = 0x9A;

    GDT[2].base = 0x0;                     // Selector 0x10 will be our data
    GDT[2].limit = 0xffffffff;
    GDT[2].type = 0x92;

    //GDT[3] = {.base=&myTss, .limit=sizeof(myTss), .type=0x89};  // You can use LTR(0x18)

    unsigned char *target = (unsigned char*) 0x130000;

    for(int i=0; i<gdt_size; i++)
    {
        sys_encode_gdt_entry(&target[i*8], GDT[i]);
    }

    set_gdt(&target[0], gdt_size * 8);
}

/*unsigned long sys_get_idt_entry_low(unsigned int addr)
{
    return (addr & 0x0000ffff) + 0x00080000;
}

unsigned long sys_get_idt_entry_high(unsigned int addr)
{
    return (addr & 0xffff0000) + 0x8e00;
}

void sys_init_idt(void)
{
    //unsigned char *target = (unsigned char*) 0x140000;

    unsigned int gdt[256];

    gdt[0] = sys_get_idt_entry_high(0x150000);
    gdt[1] = sys_get_idt_entry_low(0x150000);

    load_idt(gdt[0]);

    asm volatile ("int $0x0");
}*/



//***********************************************
//SERIAL FUNCTIONS
//***********************************************

void serial_configure_baud_rate(unsigned short com, unsigned short divisor)
{
    outb(SERIAL_LINE_COMMAND_PORT(com),
            SERIAL_LINE_ENABLE_DLAB);
    outb(SERIAL_DATA_PORT(com),
            (divisor >> 8) & 0x00FF);
    outb(SERIAL_DATA_PORT(com),
            divisor & 0x00FF);
}

//***********************************************
//SYSTEM FUNCTIONS
//***********************************************

void sys_init()
{
    serial_configure_baud_rate(SERIAL_COM1_BASE, 2);
    sys_init_gdt();
    //sys_init_idt();
}


//***********************************************
//CONVERT FUNCTIONS
//***********************************************

char *conv_num_to_str(int val, char radix)
{
    for(int i=0; i<16; i++)
    {
        char_buf[i] = 0;
    }
    int index = 14;
    int num = val;
    while(num != 0)
    {
        unsigned char d = abs(num%radix);
        char_buf[index] = (d>9)? d + 65 - 10 : d + 48;
        num /= radix;
        index--;
    }
    char_buf[index] = (val >= 0)?0x20:'-';
    char_buf[15] = STR_END;

    for(int i=0; i<16-index; i++)
    {
        char_buf[i] = char_buf[index + i];
    }

    return char_buf;
}

char *conv_unum_to_str(unsigned int val, char radix)
{
    for(int i=0; i<16; i++)
    {
        char_buf[i] = 0;
    }
    int index = 14;
    unsigned int num = val;
    while(num != 0)
    {
        unsigned char d = num%radix;
        char_buf[index] = (d>9)? d + 65 - 10 : d + 48;
        num /= radix;
        index--;
    }
    char_buf[index] = 0x20;
    char_buf[15] = STR_END;

    for(int i=0; i<16-index; i++)
    {
        char_buf[i] = char_buf[index + i];
    }

    return char_buf;
}

char *conv_int_to_str(int val)
{
    return conv_num_to_str(val, 10);
}


//////////////////////////////
//vga
//////////////////////////////

void vga_set_mode(unsigned short mode)
{
    regs16_t regs;

    // switch to graphics mode
    regs.ax = mode;
    int32(0x10, &regs);
}


//***********************************************
//FRAMEBUFFER FUNCTIONS
//***********************************************

void fb_write_symbol(unsigned int loc, char c, unsigned char bg, unsigned char fg)
{
    char *fb = (char*) 0x000B8000;

    loc *= FB_CELL_SIZE;

    fb[loc] = c;
    fb[loc + 1] = ((fg & 0x0F) << 4) | (bg & 0x0F);
}

void fb_write_string(char *str, unsigned char posx, unsigned char posy, unsigned char fg, unsigned char bg)
{
    unsigned int i = 0;
    while(str[i] != STR_END)
    {
        fb_write_symbol(FB_WIDTH * posy + posx + i, str[i], bg, fg);
        i++;
    }
}

void fb_clear_screen(unsigned char color)
{
    for(unsigned int i = 0; i< 80 * 25; i++)
    {
        fb_write_symbol(i, '*', color, color);
    }
}

void fb_move_cursor(unsigned short pos)
{
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT,    ((pos >> 8) & 0x00FF));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT,    pos & 0x00FF);
}





/*void interrupt_handler(struct cpu_state cpu, struct stack_state stack, unsigned int interrupt)
{
    char str[2] = "0\r";
    fb_write_string(str, 1, 3, 0x9, 0xF);
    //fb_clear_screen(0xF);
}*/




//***********************************************
//MAIN
//***********************************************



char *fb;

unsigned int kernel_main(void)
{
    sys_init();

    fb_clear_screen(0xF);

    char name[21] = "STRELKA System v0.1\r";
    fb_write_string(name, 80/2 - 10, 12, 0x1, 0x7);
    fb_write_string(conv_num_to_str(0x2018, 16), 0, 0, 0x1, 0x7);

    char info[60] = "F1-F4 - Change video mode. Press ENTER to continue\r";
    fb_write_string(info, 80/2 - 25, 13, 0x1, 0x7);

    //int32_test();

    char vbe_applied = 0;

    while(1)
    {
        unsigned char code = kb_read_scan_code();
        if(code != 1)
        {
            if(code == 0x1C && vbe_applied) //enter
            {
                //vbe_clear_screen(fb, 255, 255, 255);
                gui_show_hello(fb);
            }

            if(code == 0x3B) //F1
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_640X480);
                vbe_applied = 1;

                fb = (char*) vbe_info->framebuffer;

                vbe_test_draw(fb);
            }
            if(code == 0x3C)
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_800X600);
                vbe_applied = 1;

                fb = (char*) vbe_info->framebuffer;

                vbe_test_draw(fb);
            }
            if(code == 0x3D)
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_1024X768);
                vbe_applied = 1;

                fb = (char*) vbe_info->framebuffer;

                vbe_test_draw(fb);
            }
            if(code == 0x3E)
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_1280X1024);
                vbe_applied = 1;

                fb = (char*) vbe_info->framebuffer;

                vbe_test_draw(fb);
            }


            if(code == 7)
            {
                vga_set_mode(0x0013);
                char *fb = (char*) 0x000A0000;
                for(int i=0; i<320*200; i++)
                {
                    fb[i] = 1;
                }
            }
        }
        else
        {
            vbe_back_to_text_mode();
            fb_clear_screen(0x0);
            char strExit[9] = "Shutdown\r";
            fb_write_string(strExit, 36, 12, 0x7, 0x0);
            break;
        }
    }

    return 0xCAFEFACA;
    //return sys_memory_size();
    //return vbe_set_mode(VBE_STANDARD_MODE);
}
