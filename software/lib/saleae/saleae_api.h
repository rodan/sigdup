
#ifndef __SALEAE_API_H__
#define __SALEAE_API_H__

#define MAX_CHANNELS  16

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

/**

 tt transition time
 */
struct saleae_context {
    char *data[MAX_CHANNELS];
    size_t data_len[MAX_CHANNELS];
    saleae_bh0_t *hdr[MAX_CHANNELS];
    double *tt[MAX_CHANNELS];
    uint64_t loaded_channels;
};

typedef struct saleae_context saleae_context_t;

int32_t saleae_init_context(const char *prefix, saleae_context_t **context);
void saleae_free_context(saleae_context_t **context);
void saleae_print_context(saleae_context_t *ctx);

#endif
