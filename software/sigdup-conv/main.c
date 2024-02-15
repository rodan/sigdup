
// convert digital signal capture files from one format to another
//
//  source: https://github.com/rodan/sigdup
//  author: Petre Rodan <petre.rodan@subdimension.ro>
// license: GNU GPLv3

#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <fts.h>
#include <zip.h>

#include "version.h"
#include "tlpi_hdr.h"
#include "saleae_api.h"

#define          BUF_SIZE  1024000
#define     OPMODE_NORMAL  0x01
#define    OPMODE_ANALYZE  0x02

void show_usage(void);
void show_version(void);

void show_usage(void)
{
    printf("Usage: sigdup-conv [OPTION]...\n");
    printf("Convert from saleae binary capture to PulseView capture file\n\n");
    printf("mandatory options:\n");
    printf(" -i [FILE]   input saleae prefix\n");
    printf(" -o [FILE]   output sr file\n");
    printf("or\n");
    printf(" -a [FILE]   analyze replay file\n");
    printf("\n");
    printf("non-mandatory options:\n");
    printf(" -h          display this help and exit\n");
    printf(" -v          output version information and exit\n");
}

void show_version(void)
{
    printf("sigdup-conv %d.%d\nbuild %d commit %d\n", VER_MAJOR, VER_MINOR, BUILD, COMMIT);
}

int main(int argc, char *argv[])
{
    char *infile = NULL;
    char *outfile = NULL;
    int opt;
    uint8_t opmode = OPMODE_NORMAL;
    saleae_context_t *ctx;
    int32_t ret;

    while ((opt = getopt(argc, argv, "hvi:o:a:")) != -1) {
        switch (opt) {
        case 'i':
            infile = optarg;
            break;
        case 'a':
            opmode = OPMODE_ANALYZE;
            infile = optarg;
            break;
        case 'h':
            show_usage();
            exit(0);
            break;
        case 'v':
            show_version();
            exit(0);
            break;
        case 'o':
            outfile = optarg;
            break;
        default:
            break;
        }
    }

    if (opmode == OPMODE_NORMAL) {
        if ((infile == NULL) || (outfile == NULL)) {
            printf("Error: provide input and output files\n");
            show_usage();
            exit(1);
        }
    } else if (opmode == OPMODE_ANALYZE) {
        if (infile == NULL) {
            printf("Error: provide input file\n");
            show_usage();
            exit(1);
        }
    }

    if (opmode == OPMODE_ANALYZE) {
        ret = saleae_init_context(infile, &ctx);

        fprintf(stderr, "found %d signals\n", ret);
        saleae_print_context(ctx);
        saleae_free_context(&ctx);
        return 0;
    }

}

#if 0
static int recursive_unlink(const char *dir)
{
    int ret = 0;
    FTS *ftsp = NULL;
    FTSENT *curr;

    // Cast needed (in C) because fts_open() takes a "char * const *", instead
    // of a "const char * const *", which is only allowed in C++. fts_open()
    // does not modify the argument.
    char *files[] = { (char *)dir, NULL };

    // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
    //                in multithreaded programs
    // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
    //                of the specified directory
    // FTS_XDEV     - Don't cross filesystem boundaries
    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
        errExit("fts_open failed");
    }

    while ((curr = fts_read(ftsp))) {
        switch (curr->fts_info) {
        case FTS_NS:
        case FTS_DNR:
        case FTS_ERR:
            fprintf(stderr, "%s: fts_read error: %s\n", curr->fts_accpath, strerror(curr->fts_errno));
            break;

        case FTS_DC:
        case FTS_DOT:
        case FTS_NSOK:
            // Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
            // passed to fts_open()
            break;

        case FTS_D:
            // Do nothing. Need depth-first search, so directories are deleted
            // in FTS_DP
            break;

        case FTS_DP:
        case FTS_F:
        case FTS_SL:
        case FTS_SLNONE:
        case FTS_DEFAULT:
            if (remove(curr->fts_accpath) < 0) {
                fprintf(stderr, "%s: Failed to remove: %s\n", curr->fts_path, strerror(curr->fts_errno));
                ret = -1;
            }
            break;
        }
    }

    if (ftsp) {
        fts_close(ftsp);
    }

    return ret;
}
#endif
