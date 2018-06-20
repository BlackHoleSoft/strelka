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

/*#define VBE_STANDARD_MODE               0x112
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

#define FONT_LETTER_SPACE               10

#define FONT_BM_HEIGHT                     14
#define FONT_BM_WIDTH                      16

#define STR_END                 '\r'*/


char char_buf[256];


void set_gdt(unsigned char * gdt, unsigned short size);
//void load_idt(unsigned int addr);
void outb(unsigned short port, unsigned char data);
unsigned char inb(unsigned short port);
void bios_test(void);
