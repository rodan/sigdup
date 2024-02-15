#ifndef __SALEAE_API_H__
#define __SALEAE_API_H__

#define      MAX_CHANNELS  16
#define   SAMPLING_ERR_NS  3

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

struct saleae_channel {
    saleae_bh0_t hdr;   // header
    uint64_t *ts;       // picosecond timestamps
    uint64_t *dts;      // differential timestamps
    uint8_t *raw;
    size_t raw_len;
};

typedef struct saleae_channel saleae_channel_t;

struct saleae_context {
    saleae_channel_t ch[MAX_CHANNELS];
    uint64_t loaded_channels;
};

typedef struct saleae_context saleae_context_t;

int32_t saleae_init_context(const char *prefix, saleae_context_t **context);
void saleae_free_context(saleae_context_t **context);
void saleae_print_context(saleae_context_t *ctx);

#endif
