#ifndef SDNAVIGATION_H
#define SDNAVIGATION_H 



#define  INK_BLACK 0x00
#define  INK_BLUE 0x01
#define  INK_RED 0x02
#define  INK_MAGENTA 0x03
#define  INK_GREEN 0x04
#define  INK_CYAN 0x05
#define  INK_YELLOW 0x06
#define  INK_WHITE 0x07
#define  PAPER_BLACK (INK_BLACK<<3)
#define  PAPER_BLUE (INK_BLUE<<3)
#define  PAPER_RED (INK_RED<<3)
#define  PAPER_MAGENTA (INK_MAGENTA<<3)
#define  PAPER_GREEN (INK_GREEN<<3)
#define  PAPER_CYAN (INK_CYAN<<3)
#define  PAPER_YELLOW (INK_YELLOW<<3)
#define  PAPER_WHITE (INK_WHITE<<3)
#define COLOR_BRIGHT 64
#define COLOR_BLINK 128


#define COLOR_TEXT (COLOR_BRIGHT  | PAPER_BLACK | INK_WHITE)
#define COLOR_FILE (COLOR_BRIGHT  | PAPER_BLACK | INK_YELLOW)
#define COLOR_FILE_SELECTED (COLOR_BRIGHT  | PAPER_YELLOW | INK_BLACK)
#define COLOR_ERROR (COLOR_BRIGHT  | COLOR_BLINK | PAPER_BLACK | INK_RED)

void sdNavigationSetup(void);
int sdNavigation(boolean);
void sdNavigationPrintNumber(int indx, int color);  
void sdNavigationPrintNumberBig(int indx, int color);  
int sdNavigationFileSave(char *filename);
int sdNavigationGetFileName(char *filename);


void   sdNavigationClearLine(int line, int color);
void   sdNavigationPixMem(int x, int y, int *bmp, int *col);
void   sdNavigationCls(int col);
void   sdNavigationPrintCh(int x, int y, unsigned char c, int col);
void   sdNavigationPrintStr(int x, int y, char *str, int col);
void   sdNavigationPrintFStr(int x, int y, char *str, int col);
void   sdNavigationPrintChBig(int x, int y, unsigned char c, int col);
void   sdNavigationPrintStrBig(int x, int y, char *str, int col);
void   sdNavigationPrintFStrBig(int x, int y, char *str, int col);



#endif
