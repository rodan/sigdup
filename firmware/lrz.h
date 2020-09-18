#ifndef __LRZ_H__
#define __LRZ_H__

typedef struct {
    uint8_t state;
} rz_sm_t;

enum rz_states {
    RZ_INIT,
    RZ_INIT_2,
    RZ_INIT_3,
    RZ_POS,
    RZ_RCV,
    RZ_FIN
};

uint8_t rzfile_sm(void);
void z_rst_zi(void);
int wcreceive(void);

#endif
