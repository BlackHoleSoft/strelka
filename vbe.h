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

#define FONT_LETTER_SPACE               10

#define FONT_BM_HEIGHT                     14
#define FONT_BM_WIDTH                      16

#define STR_END                 '\r'

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


int vbe_current_mode;
int vbe_width;
int vbe_height;
int vbe_bpp;
int vbe_pitch;

void vbe_back_to_text_mode(void);
vbe_mode_info_structure *vbe_set_mode(unsigned short mode);
void vbe_draw_pixel(char *fb, unsigned char r, unsigned char g, unsigned char b, int x, int y);
void vbe_clear_screen(char *fb, unsigned char r, unsigned char g, unsigned char b);
void vbe_draw_rect(char *fb, unsigned char r, unsigned char g, unsigned char b, int x, int y, int w, int h);
void vbe_draw_border(char *fb, unsigned char r, unsigned char g, unsigned char b, int x, int y, int w, int h, int bw);
void vbe_draw_char(char *fb, int letter, unsigned char r, unsigned char g, unsigned char b, int x, int y);
void vbe_draw_string(char *fb, char *str, unsigned char r, unsigned char g, unsigned char b, int x, int y);
void vbe_draw_string_shadow(char *fb, char *str, unsigned char r, unsigned char g, unsigned char b, int x, int y);
void vbe_test_draw(char *fb);
