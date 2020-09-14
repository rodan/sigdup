#ifndef __UI_H__
#define __UI_H__

#include "proj.h"

void display_menu(void);

void parse_user_input(void);

void print_buf(uint8_t *data, const uint16_t size);
void print_buf_ascii2mem(uint8_t *data, const uint16_t size);
void print_buf_array(uint8_t *data, const uint16_t size);

void display_DONE(void);
void display_SEP(void);
void display_PASSED(void);
void display_FAILED(void);

#endif
