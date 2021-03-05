#ifndef __FRAM_GLUE_H__
#define __FRAM_GLUE_H__

#include "proj.h"

typedef struct {
    uint32_t file_start;
    uint32_t file_sz;
} fram_header;

extern fram_header fram_hdr;

void fram_init(void);
int8_t fram_seek(const uint32_t address);
int16_t fram_read(uint8_t *buf, const uint16_t count);
int16_t fram_write(uint8_t *buf, const uint16_t count);
int8_t fram_write_header(const fram_header *hdr);
fram_header *fram_get_hdr_p(void);

#endif
