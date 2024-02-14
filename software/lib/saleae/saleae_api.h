
#ifndef __SALEAE_API_H__
#define __SALEAE_API_H__

// version 0 of the saleae binary header
struct __attribute__((packed)) saleae_bh0 {
    uint8_t identifier[8];
    int32_t version;
    int32_t type;
    uint32_t initial_state;
    double begin_time;
    double end_time;
    uint64_t num_transitions;
};

typedef struct saleae_bh0 saleae_bh0_t;

#endif
