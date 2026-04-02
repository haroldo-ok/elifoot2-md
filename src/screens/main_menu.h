#ifndef ELF_MAIN_MENU_H
#define ELF_MAIN_MENU_H
#include <genesis.h>

#define MENU_RESULT_SQUAD      0u
#define MENU_RESULT_FINANCES   1u
#define MENU_RESULT_PLAY       2u
#define MENU_RESULT_TRANSFERS  3u
#define MENU_RESULT_COACHES    4u
#define MENU_RESULT_CUP        5u
#define MENU_RESULT_STANDINGS  6u
#define MENU_RESULT_PALMARES   7u
#define MENU_RESULT_SAVE       8u
#define MENU_RESULT_MARCADORES 9u
#define MENU_RESULT_PROXIMAS   10u
#define MENU_RESULT_RESULTADOS 11u
#define MENU_RESULT_CALENDARIO 12u
#define MENU_RESULT_ULTIMAS_T  13u
#define MENU_RESULT_BACK       0xFFu

u8 screen_main_menu(void);
#endif
