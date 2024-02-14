
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "saleae_api.h"

#ifndef MAX_PATH
#define           MAX_PATH  4096
#endif

int32_t saleae_init_context(const char *prefix, saleae_context_t **context)
{
    uint8_t i;
    uint8_t ret = 0;
    char *fname;
    int fd;
    struct stat st;
    saleae_context_t *ctx = *context;

    fname = (char *) calloc(MAX_PATH, sizeof(char));
    if (!fname)
        return -1;

    ctx = (saleae_context_t *) calloc(1, sizeof(saleae_context_t));
    if (!ctx)
        goto error;

    for (i=0; i<MAX_CHANNELS; i++) {
        snprintf(fname, MAX_PATH, "%s_%d.bin", prefix, i);
        if (stat(fname, &st) < 0) {
            //fprintf(stderr, "cannot find %s\n", fname);
        } else {
            fd = open(fname, O_RDONLY);
            if (fd == -1)
                goto error;

            ctx->data[i] = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (ctx->data[i] == MAP_FAILED)
                goto error;
            ctx->data_len[i] = st.st_size;
            ctx->loaded_channels |= 1 << i;
            ctx->hdr[i] = (struct saleae_bh0 *) ctx->data[i];
            ret++;
        }
    }

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
        for (i=0; i<MAX_CHANNELS; i++) {
            if (ctx->data_len[i]) {
                munmap(ctx->data[i], ctx->data_len[i]);
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

    for (i=0; i<MAX_CHANNELS; i++) {
        if (ctx->loaded_channels & (1 << i)) {
            fprintf(stdout, "channel %d\n", i);
            fprintf(stdout, "  version %d\n", ctx->hdr[i]->version);
            fprintf(stdout, "  type %d\n", ctx->hdr[i]->version);
            fprintf(stdout, "  initial state %d\n", ctx->hdr[i]->initial_state);
            fprintf(stdout, "  begin time %.16lf\n", ctx->hdr[i]->begin_time);
            fprintf(stdout, "  end time   %.16lf\n", ctx->hdr[i]->end_time);
            fprintf(stdout, "  transitions %ld\n", ctx->hdr[i]->num_transitions);
            p = (double *)((char *) (ctx->data[i] + sizeof(struct saleae_bh0)));

            for (t=0; t<ctx->hdr[i]->num_transitions; t++) {
                fprintf(stdout, " %lf", *p);
                p++;
            }
            fprintf(stdout, "\n");
        }
    }
}
