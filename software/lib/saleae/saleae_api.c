
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#ifdef CONFIG_OMP
#include <omp.h>
#endif

#include "saleae_api.h"

#ifndef MAX_PATH
#define MAX_PATH  4096
#endif

const uint64_t samplerates[] = {
    40000000,                   // 25kHz
    20000000,                   // 50kHz
    10000000,                   // 100kHz
    5000000,                    // 200kHz
    4000000,                    // 250kHz
    2000000,                    // 500kHz
    1000000,                    // 1MHz
    500000,                     // 2MHz
    400000,                     // 2.5MHz
    250000,                     // 4MHz
    200000,                     // 5MHz
    160000,                     // 6.25MHz
    125000,                     // 8MHz
    100000,                     // 10MHz
    83333,                      // 12MHz
    80000,                      // 12.5MHz
    62500,                      // 16MHz
    50000,                      // 20MHz
    41667,                      // 24MHz
    40000,                      // 25MHz
    20000,                      // 50MHz
    10000,                      // 100MHz
    8000,                       // 125MHz
    4000,                       // 250MHz
    2000,                       // 500MHz
};

int32_t saleae_rewrite_transitions(saleae_channel_t *ch)
{
    uint64_t i;
    double *p;
    uint64_t start, current, last;

    if (!ch->raw_len)
        return -1;

    p = (double *)((char *)(ch->raw + sizeof(struct saleae_bh0)));
    start = *p * 1000000000000UL;       // convert to picoseconds
    last = start;

    for (i = 0; i < ch->hdr.num_transitions; i++) {
        current = *p * 1000000000000UL; // convert to picoseconds
        ch->ts[i] = current - start;
        ch->dts[i] = current - last;
        last = current;
        p++;
    }

    for (i = 0; i < ch->hdr.num_transitions; i++) {
        fprintf(stdout, " %lf", *p);
        p++;
    }
    fprintf(stdout, "\n");
    for (i = 0; i < ch->hdr.num_transitions; i++) {
        fprintf(stdout, " %ld", ch->ts[i]);
    }
    fprintf(stdout, "\n");
    for (i = 0; i < ch->hdr.num_transitions; i++) {
        fprintf(stdout, " %ld", ch->dts[i]);
    }
    fprintf(stdout, "\n\n\n");

    return 0;
}

int32_t saleae_init_context(const char *prefix, saleae_context_t **context)
{
    uint8_t i;
    uint8_t ret = 0;
    char *fname;
    int fd;
    struct stat st;
    saleae_context_t *ctx = *context;

    fname = (char *)calloc(MAX_PATH, sizeof(char));
    if (!fname)
        return -1;

    ctx = (saleae_context_t *) calloc(1, sizeof(saleae_context_t));
    if (!ctx)
        goto error;

    for (i = 0; i < MAX_CHANNELS; i++) {
        snprintf(fname, MAX_PATH, "%s_%d.bin", prefix, i);
        if (stat(fname, &st) == 0) {
            fd = open(fname, O_RDONLY);
            if (fd == -1)
                goto error;

            ctx->ch[i].raw = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (ctx->ch[i].raw == MAP_FAILED)
                goto error;
            ctx->ch[i].raw_len = st.st_size;

            memcpy(&ctx->ch[i].hdr, ctx->ch[i].raw, sizeof(saleae_bh0_t));

            ctx->ch[i].ts = (uint64_t *) calloc(ctx->ch[i].hdr.num_transitions, sizeof(uint64_t));
            if (!ctx->ch[i].ts)
                goto error;

            ctx->ch[i].dts = (uint64_t *) calloc(ctx->ch[i].hdr.num_transitions, sizeof(uint64_t));
            if (!ctx->ch[i].dts)
                goto error;

            ctx->loaded_channels |= 1 << i;
            ret++;
        }
    }

#if defined CONFIG_OMP
    omp_set_num_threads(CONFIG_OMP_THREADS);

#pragma omp parallel
    {
        uint32_t idx;

        int t_cnt = omp_get_num_threads();
        int t_cur = omp_get_thread_num();

        for (idx = t_cur; idx < MAX_CHANNELS; idx += t_cnt) {
            if (ctx->ch[idx].raw_len)
                saleae_rewrite_transitions(&ctx->ch[idx]);
        }
    }
#else
    // single thread
    for (i = 0; i < MAX_CHANNELS; i++) {
        if (ctx->ch[i].raw_len)
            saleae_rewrite_transitions(&ctx->ch[i]);
    }
#endif

 error:
    free(fname);

    *context = ctx;
    return ret;
}

void saleae_free_context(saleae_context_t **context)
{
    uint8_t i;
    saleae_context_t *ctx = *context;

    if (*context) {
        for (i = 0; i < MAX_CHANNELS; i++) {
            if (ctx->ch[i].raw_len) {
                munmap(ctx->ch[i].raw, ctx->ch[i].raw_len);
                if (ctx->ch[i].ts)
                    free(ctx->ch[i].ts);
                if (ctx->ch[i].dts)
                    free(ctx->ch[i].dts);
            }
        }
        free(*context);
    }
}

void saleae_print_context(saleae_context_t *ctx)
{
    uint8_t i;
    uint64_t t;
    double *p;

    for (i = 0; i < MAX_CHANNELS; i++) {
        if (ctx->loaded_channels & (1 << i)) {
            fprintf(stdout, "channel %d\n", i);
            fprintf(stdout, "  version %d\n", ctx->ch[i].hdr.version);
            fprintf(stdout, "  type %d\n", ctx->ch[i].hdr.version);
            fprintf(stdout, "  initial state %d\n", ctx->ch[i].hdr.initial_state);
            fprintf(stdout, "  begin time %.16lf\n", ctx->ch[i].hdr.begin_time);
            fprintf(stdout, "  end time   %.16lf\n", ctx->ch[i].hdr.end_time);
            fprintf(stdout, "  transitions %ld\n", ctx->ch[i].hdr.num_transitions);
            //p = (double *)((char *) (ctx->ch[i].raw + sizeof(struct saleae_bh0)));

            //for (t=0; t<ctx->ch[i].hdr.num_transitions; t++) {
            //    fprintf(stdout, " %lf", *p);
            //    p++;
            //}
            fprintf(stdout, "\n");
        }
    }
}
