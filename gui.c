#include "gui.h"
#include "vbe.h"
#include "input.h"
//#include <math.h>


//**************************
//GUI LIBRARY
//**************************

typedef struct __attribute__((packed)) {
    char *text;
    int x;
    int y;
    int r;
    int g;
    int b;
} GuiLabel;

void gui_update_label(char *fb, GuiLabel *lb)
{
    vbe_draw_string(fb, lb->text, lb->r, lb->g, lb->b, lb->x, lb->y);
}

typedef struct __attribute__((packed)) {
    char *text;
    int x;
    int y;
    int width;
    int height;
    int fgr;
    int fgg;
    int fgb;
    int bgr;
    int bgg;
    int bgb;
} GuiTextField;

void gui_update_text_field(char *fb, unsigned char keycode, GuiTextField *tf)
{
    vbe_draw_rect(fb, 170, 170, 170, tf->x+2, tf->y+2, tf->width, tf->height);
    vbe_draw_rect(fb, tf->bgr, tf->bgg, tf->bgb, tf->x, tf->y, tf->width, tf->height);
    vbe_draw_border(fb, tf->bgr/2, tf->bgg/2, tf->bgb/2, tf->x, tf->y, tf->width, tf->height, 1);

    int textlen = 0;
    for(textlen=0; tf->text[textlen] != '\r' && textlen<100-1; textlen++)
    {

    }
    if(textlen < 99 && textlen < tf->width/FONT_LETTER_SPACE && keycode < 0x58)
    {
        if(keycode == 0x0E && textlen > 0) //backspace
        {
            tf->text[textlen] = 0;
            tf->text[textlen-1] = '\r';
        }
        else
        {
            tf->text[textlen] = input_get_key(keycode);
            tf->text[textlen+1] = '\r';
        }
    }

    vbe_draw_string(fb, tf->text, tf->fgr, tf->fgg, tf->fgb, tf->x, tf->y);
}


//**************************
//MENU
//**************************

void gui_update(char *fb, unsigned char keycode)
{
    GuiLabel lb1[128];
    lb1->text = "Hello!!!\r";
    lb1->x = 100;
    lb1->y = 100;
    lb1->r = lb1->g = lb1->b = 70;

    GuiTextField tf1[128];
    tf1->text = "Input\r";
    tf1->x = 200;
    tf1->y = 100;
    tf1->width = 290;
    tf1->height = 20;
    tf1->bgr = tf1->bgg = tf1->bgb = 255;
    tf1->fgr = tf1->fgg = tf1->fgb = 70;

    gui_update_label(fb, lb1);
    gui_update_text_field(fb, keycode, tf1);
}

void gui_loop_iter(char *fb, unsigned char keycode)
{
    vbe_clear_screen(fb, 255, 255, 255);

    gui_update(fb, keycode);
}

void gui_main_loop(char *fb)
{
    unsigned char code = 0;
    unsigned char lastcode = 0;

    while(code != 1)
    {
        code = kb_read_scan_code();

        if(code != lastcode)
        {
            gui_loop_iter(fb, code);
        }

        lastcode = code;
    }
}

void gui_show_hello(char *fb)
{
    vbe_clear_screen(fb, GUI_BG_R, GUI_BG_G, GUI_BG_B);
    char logo[] = "STRELKA\r";
    vbe_draw_string(fb, logo, 255, 255, 255, vbe_width/2 - 4*FONT_LETTER_SPACE, vbe_height/2 - 40);
    double max = 1;
    for(double i=0; i<max; i+=0.001)
    {
        vbe_draw_rect(fb, 255, 10+i/max*200, 0+i/max*200, 0, vbe_height/2 - 20, i/max*vbe_width, 40);
    }
    gui_main_loop(fb);
}
