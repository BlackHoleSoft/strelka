#include "vbe.h"
#include "font.h"


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

void vbe_draw_string_shadow(char *fb, char *str, unsigned char r, unsigned char g, unsigned char b, int x, int y)
{
    vbe_draw_string(fb, str, 140, 140, 140, x+2, y+2);
    vbe_draw_string(fb, str, r, g, b, x, y);
}

void vbe_test_draw(char *fb)
{
    vbe_clear_screen(fb, 255, 110, 100);

    /*vbe_draw_rect(fb, 255, 100, 200, 100, 100, 80, 50);
    vbe_draw_border(fb, 100, 100, 100, 100, 100, 80, 50, 4);

    vbe_draw_rect(fb, 100, 255, 130, 150, 130, 50, 80);
    vbe_draw_border(fb, 200, 200, 200, 150, 130, 50, 80, 1);

    vbe_draw_char(fb, 1, 255, 255, 255, 10, 10);
    vbe_draw_char(fb, 16, 255, 255, 255, 30, 10);
    vbe_draw_char(fb, 33, 255, 255, 255, 50, 10);
    vbe_draw_char(fb, 49, 255, 255, 255, 70, 10);*/

    char str1[] = "STRELKA operating system. VBE mode.\r";
    char str2[] = "Press F1-F4 to change video mode. Press ENTER to continue\r";

    vbe_draw_string(fb, str1, 255, 255, 255, vbe_width/2 - 17*FONT_LETTER_SPACE, vbe_height/2 - 10);
    vbe_draw_string(fb, str2, 255, 255, 255, vbe_width/2 - 29*FONT_LETTER_SPACE, vbe_height/2 + 10);
}
