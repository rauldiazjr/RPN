#ifndef _LCD_H
#define _LCD_H

#define MAX_CHAR_IN_ONE_LINE 16

enum ROW_NUMBERS
{
  LINE1,
  LINE2	
};

void init_lcd(void);
void lcd_putstring(unsigned char line, char *string);
void lcd_clear(void);
void lcd_backlight_on(void);
int lcd_gotoxy(unsigned int x, unsigned int y);
void lcd_putchar(int c);

#endif
