#include "font.h"

/* The I/O ports */
#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

#define FB_CELL_SIZE            2
#define FB_WIDTH                80
#define FB_HEIGHT               25


#define SERIAL_COM1_BASE                0x3F8      /* COM1 base port */

#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)

/* SERIAL_LINE_ENABLE_DLAB:
    * Tells the serial port to expect first the highest 8 bits on the data port,
    * then the lowest 8 bits will follow
    */
#define SERIAL_LINE_ENABLE_DLAB         0x80

#define VBE_STANDARD_MODE               0x112
#define VBE_LARGE_MODE                  0x118

#define VBE_MODE_1024X768               0x118
#define VBE_MODE_640X480                0x112
#define VBE_MODE_800X600                0x115
#define VBE_MODE_1280X1024              0x11B

#define VBE_STANDARD_W                  640
#define VBE_STANDARD_H                  480

#define VBE_LARGE_W                     1024
#define VBE_LARGE_H                     768

#define FONT_HEIGHT                     16
#define FONT_WIDTH                      14

#define FONT_LETTER_SPACE               12

#define FONT_BM_HEIGHT                     14
#define FONT_BM_WIDTH                      16

#define STR_END                 '\r'

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


char char_buf[256];


void set_gdt(unsigned char * gdt, unsigned short size);
//void load_idt(unsigned int addr);
void outb(unsigned short port, unsigned char data);
unsigned char inb(unsigned short port);
void bios_test(void);

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



typedef struct __attribute__ ((packed)) {
    unsigned short di, si, bp, sp, bx, dx, cx, ax;
    unsigned short gs, fs, es, ds, eflags;
} regs16_t;

typedef struct __attribute__ ((packed)) {
	unsigned short attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	unsigned char window_a;			// deprecated
	unsigned char window_b;			// deprecated
	unsigned short granularity;		// deprecated; used while calculating bank numbers
	unsigned short window_size;
	unsigned short segment_a;
	unsigned short segment_b;
	unsigned int win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	unsigned short pitch;			// number of bytes per horizontal line
	unsigned short width;			// width in pixels
	unsigned short height;			// height in pixels
	unsigned char w_char;			// unused...
	unsigned char y_char;			// ...
	unsigned char planes;
	unsigned char bpp;			// bits per pixel in this mode
	unsigned char banks;			// deprecated; total number of banks in this mode
	unsigned char memory_model;
	unsigned char bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	unsigned char image_pages;
	unsigned char reserved0;

	unsigned char red_mask;
	unsigned char red_position;
	unsigned char green_mask;
	unsigned char green_position;
	unsigned char blue_mask;
	unsigned char blue_position;
	unsigned char reserved_mask;
	unsigned char reserved_position;
	unsigned char direct_color_attributes;

	unsigned int framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	unsigned int off_screen_mem_off;
	unsigned short off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	unsigned char reserved1[206];
}  vbe_mode_info_structure;

// tell compiler our int32 function is external
extern void int32(unsigned char intnum, regs16_t *regs);

// int32 test
void int32_test()
{
    regs16_t regs;

    // switch to 320x200x256 graphics mode
    regs.ax = 0x0013;
    int32(0x10, &regs);

    // full screen with blue color (1)
    char *buf = (char *)0xA0000;
    for(int i=0; i<320*200; i++)
    {
        buf[i] = 1;
    }

    // wait for key
    regs.ax = 0x0000;
    int32(0x16, &regs);

    // switch to 80x25x16 text mode
    regs.ax = 0x0003;
    int32(0x10, &regs);
}




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

//***********************************************
//KEYBOARD FUNCTIONS
//***********************************************

unsigned char kb_read_scan_code(void)
{
    return inb(0x60);
}



void vga_set_mode(unsigned short mode)
{
    regs16_t regs;

    // switch to graphics mode
    regs.ax = mode;
    int32(0x10, &regs);
}





//***********************************************
//VBE FUNCTIONS
//***********************************************

int vbe_current_mode;
int vbe_width;
int vbe_height;
int vbe_bpp;
int vbe_pitch;


void vbe_back_to_text_mode(void)
{
    regs16_t regs;
    regs.ax = 0x0003;
    int32(0x10, &regs);
}

void vbe_draw_pixel(char *fb, unsigned char r, unsigned char g, unsigned char b, int x, int y)
{
    int i = y*vbe_pitch + x*vbe_bpp;
    /*fb[i+vbe_bpp-1] = r;
    fb[i+vbe_bpp-2] = g;
    fb[i+vbe_bpp-3] = b;*/
    fb[i] = b;
    fb[i+1] = g;
    fb[i+2] = r;
}

void vbe_clear_screen(char *fb, unsigned char r, unsigned char g, unsigned char b)
{
    for(int i=0; i<vbe_height*vbe_pitch; i+=vbe_bpp)
    {
        /*fb[i+vbe_bpp-1] = r;
        fb[i+vbe_bpp-2] = g;
        fb[i+vbe_bpp-3] = b;*/
        fb[i] = b;
        fb[i+1] = g;
        fb[i+2] = r;
    }
    /*for(int i=0; i<vbe_width; i++)
    {
        for(int j=0; j<vbe_height; j++)
        {
            vbe_draw_pixel(fb, r, g, b, i, j);
        }
    }*/
}

void vbe_draw_rect(char *fb, unsigned char r, unsigned char g, unsigned char b, int x, int y, int w, int h)
{
    for(int i=y; i<y+h; i++)
    {
        for(int j=x; j<x+w; j++)
        {
            vbe_draw_pixel(fb, r, g, b, j, i);
        }
    }
}

void vbe_draw_border(char *fb, unsigned char r, unsigned char g, unsigned char b, int x, int y, int w, int h, int bw)
{
    vbe_draw_rect(fb, r, g, b, x, y, bw, h);
    vbe_draw_rect(fb, r, g, b, x, y, w, bw);

    vbe_draw_rect(fb, r, g, b, x+w-bw, y, bw, h);
    vbe_draw_rect(fb, r, g, b, x, y+h-bw, w, bw);
}

/*void vbe_draw_bitmap(char *fb, char *bitmap, int x, int y, int w, int h) //argb
{
    for(int i=0; i<h; i++)
    {
        for(int j=0; j<w; j++)
        {
            vbe_draw_pixel(fb, bitmap[j*3+0], bitmap[j*3+1], bitmap[j*3+2], x+j, y+i);
        }
    }
}*/

void vbe_draw_char(char *fb, int letter, unsigned char r, unsigned char g, unsigned char b, int x, int y)
{
    int pos = (letter%FONT_BM_WIDTH + (letter/FONT_BM_WIDTH)*FONT_HEIGHT*FONT_BM_WIDTH)*FONT_WIDTH; //in bits
    for(int i=0; i<FONT_HEIGHT; i++)
    {
        //int right = pos + FONT_WIDTH;
        for(int j=0; j<FONT_WIDTH; j++)
        {
            if(((font_standart[pos/8] >> (8-1-pos%8)) & 1) == 0)
            {
                vbe_draw_pixel(fb, r, g, b, x+j, y+i);
            }
            pos++;
        }
        pos += FONT_BM_WIDTH*FONT_WIDTH - FONT_WIDTH;
    }
}

vbe_mode_info_structure *vbe_set_mode(unsigned short mode)
{
    regs16_t regs;
    regs.ax = 0x4F02;
    regs.bx = mode | (1 << 14);
    int32(0x10, &regs);

    if(regs.ax == 0x004F)
    {
        regs.ax = 0x4F01;
        regs.cx = mode;
        regs.di = 0x0000;
        regs.es = 0xA000;
        int32(0x10, &regs);
        if(regs.ax != 0x004F)
        {
            vbe_back_to_text_mode();
        }

        vbe_mode_info_structure *vbe_info = (vbe_mode_info_structure*) 0xA0000;

        vbe_width = vbe_info->width;
        vbe_height = vbe_info->height;
        vbe_bpp = vbe_info->bpp / 8;
        vbe_pitch = vbe_info->pitch;

        /*char *fb = (char*) vbe_info->framebuffer;
        for(int i=0; i<640*480*3; i++)
        {
            fb[i] = 100;
        }

        // wait for key
        regs.ax = 0x0000;
        int32(0x16, &regs);

        // switch to 80x25x16 text mode
        regs.ax = 0x0003;
        int32(0x10, &regs);*/

        return vbe_info;
    }
    else
    {
        vbe_back_to_text_mode();
    }

    vbe_mode_info_structure vbe_info;
    return &vbe_info;
}

void vbe_draw_string(char *fb, char *str, unsigned char r, unsigned char g, unsigned char b, int x, int y)
{
    unsigned int i = 0;
    while(str[i] != STR_END)
    {
        vbe_draw_char(fb, str[i]-32, r, g, b, x+i*FONT_LETTER_SPACE, y);
        i++;
    }
}

void vbe_test_draw(char *fb)
{
    vbe_draw_rect(fb, 255, 100, 200, 100, 100, 80, 50);
    vbe_draw_border(fb, 100, 100, 100, 100, 100, 80, 50, 4);

    vbe_draw_rect(fb, 100, 255, 130, 150, 130, 50, 80);
    vbe_draw_border(fb, 200, 200, 200, 150, 130, 50, 80, 1);

    /*vbe_draw_char(fb, 1, 255, 255, 255, 10, 10);
    vbe_draw_char(fb, 16, 255, 255, 255, 30, 10);
    vbe_draw_char(fb, 33, 255, 255, 255, 50, 10);
    vbe_draw_char(fb, 49, 255, 255, 255, 70, 10);*/

    //char str[11] = "Hello VBE!\r";
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

    while(1)
    {
        unsigned char code = kb_read_scan_code();
        if(code != 1)
        {
            if(code == 0x1C) //enter
            {

            }

            if(code == 0x3B) //F1
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_640X480);

                char *fb = (char*) vbe_info->framebuffer;
                vbe_clear_screen(fb, 170, 165, 210);

                vbe_draw_string(fb, conv_unum_to_str(vbe_info->framebuffer, 16), 100, 100, 255, 10, 30);
                vbe_test_draw(fb);
            }
            /*if(code == 0x3C)
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_800X600);

                char *fb = (char*) vbe_info->framebuffer;
                vbe_clear_screen(fb, 170, 165, 210);

                vbe_test_draw(fb);
            }
            if(code == 0x3D)
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_1024X768);

                char *fb = (char*) vbe_info->framebuffer;
                vbe_clear_screen(fb, 170, 165, 210);

                vbe_test_draw(fb);
            }
            if(code == 0x3E)
            {
                vbe_mode_info_structure *vbe_info = vbe_set_mode(VBE_MODE_1280X1024);

                char *fb = (char*) vbe_info->framebuffer;
                vbe_clear_screen(fb, 170, 165, 210);

                vbe_test_draw(fb);
            }

            if(code == 5)
            {
                int32_test();
                fb_clear_screen(0xF);
                char str[6] = "Hello\r";
                fb_write_string(str, 0, 0, 0x9, 0xF);
            }
            if(code == 7)
            {
                vga_set_mode(0x0013);
                char *fb = (char*) 0x000A0000;
                for(int i=0; i<320*200; i++)
                {
                    fb[i] = 1;
                }
            }*/
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
