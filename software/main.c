
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <fts.h>
#include <zip.h>

#include "version.h"
#include "config.h"
#include "lib_convert.h"
#include "tlpi_hdr.h"
#include "ini.h"
#include "zcrc.h"
#include "list.h"
#include "pg.h"
#include "sig_mng.h"

#define          BUF_SIZE  1024000
#define           MAXPATH  256
#define        DEF_DEVICE  "device 1"
#define   DEF_CLK_DIVIDER  CLK_DIV_16

const uint8_t clk_dividers[CLK_DIV_CNT] = { CLK_DIV_1, CLK_DIV_2, CLK_DIV_3, CLK_DIV_4, CLK_DIV_5, CLK_DIV_6,
    CLK_DIV_7, CLK_DIV_8, CLK_DIV_10, CLK_DIV_12, CLK_DIV_14, CLK_DIV_16, CLK_DIV_20, CLK_DIV_24,
    CLK_DIV_28, CLK_DIV_32, CLK_DIV_40, CLK_DIV_48, CLK_DIV_56, CLK_DIV_64
};

static int metadata_parser(void *user, const char *section, const char *name, const char *value);
static int recursive_unlink(const char *dir);

char *device;                   /// device name as used in the metadata file
uint8_t clk_divider = 0;

static uint16_t mask = 0xffff;  /// mask to be applied to the captured signal - set bitmask 1 to channels that are of interest. default 0xffff
static uint8_t lshift = 0;      /// left shift input signal by these many bits, so that a channel of interest lands on a particular pin on ports P3
static uint8_t rshift = 0;      /// right shift input signal by these many bits, so that a channel of interest lands on a particular pin on ports P3

#define     POLYNOMIAL_32  0xEDB88320
#define     OPMODE_NORMAL  0x01
#define    OPMODE_ANALYZE  0x02

// Holds a crc32 lookup table
uint32_t crc32Table[256];
uint8_t crc32TableInit = 0;
// crc32 used in the zcrc library
void initSwCrc32Table(void);
void show_usage(void);
void show_version(void);

LIST(replay);

void show_usage(void)
{
    uint8_t c;

    printf("Usage: pg [OPTION]...\n");
    printf("Convert PulseView capture into a highly-compressed file used to replay the signals\n\n");
    printf("mandatory options:\n");
    printf(" -i [FILE]   input PulseView '.sr' file\n");
    printf(" -o [FILE]   output replay file\n");
    printf("or\n");
    printf(" -a [FILE]   analyze replay file\n");
    printf("\n");
    printf("non-mandatory options:\n");
    printf(" -k [NAME]   pulseview metadata name for input file, default: '%s'\n", DEF_DEVICE);
    printf(" -m [HEX]    mask (in hex) to be applied to the input port, default: 0x%x\n", mask);
    printf(" -l [DEC]    number of bits the output signal is shifted to the left, default: %d\n", lshift);
    printf(" -r [DEC]    number of bits the output signal is shifted to the right, default: %d\n", rshift);
    printf(" -d [DEC]    force timer clock divider. can be one of the following numbers:\n");
    printf("               ");
    for (c = 0; c < CLK_DIV_CNT; c++) {
        printf("%u,", clk_dividers[c]);
    }
    printf(" default: auto\n");
    printf(" -h          display this help and exit\n");
    printf(" -v          output version information and exit\n");
}

void show_version(void)
{
    printf("pg %d.%d\nbuild %d commit %d\n", VER_MAJOR, VER_MINOR, BUILD, COMMIT);
}

int main(int argc, char *argv[])
{
    int fdin, fdout;
    char *infile = NULL;
    char *outfile = NULL;
    int opt;
    char buf[BUF_SIZE];
    uint16_t sig16;
    uint32_t cnt, c, rcnt = 0;
    uint64_t i;
    uint8_t opmode = OPMODE_NORMAL;
    uint8_t clk_divider_opt;

    // zip related
    struct zip *za;
    struct zip_file *zf;
    struct zip_stat sb;
    int err;
    int len;
    int fd;
    long long sum;
    char temp_file[MAXPATH];
    char *temp_dir;
    char temp_template[] = "/tmp/pg.XXXXXX";
    uint32_t freq_multiplier = 0;
    uint32_t freq = 0;
    struct stat st;
    uint8_t *sig_8ch;
    uint8_t *sig_replay;
    uint16_t chunk;
    uint16_t capture_chunks;
    uint32_t capture_size_total = 0;

    replay_header_t replay_header;
    list_init(replay);

    input_sig_t s;

    device = DEF_DEVICE;

    while ((opt = getopt(argc, argv, "hvb:i:o:d:m:l:r:a:k:")) != -1) {
        switch (opt) {
        case 'm':
            hstr_to_uint16(optarg, &mask, 0, strlen(optarg) - 1, 0, -1);
            break;
        case 'l':
            hstr_to_uint8(optarg, &lshift, 0, strlen(optarg) - 1, 0, -1);
            break;
        case 'r':
            hstr_to_uint8(optarg, &rshift, 0, strlen(optarg) - 1, 0, -1);
            break;
        case 'd':
            clk_divider_opt = atoi(optarg);
            for (i = 0; i < CLK_DIV_CNT; i++) {
                if (clk_divider_opt == clk_dividers[i]) {
                    clk_divider = clk_divider_opt;
                }
            }
            if (clk_divider == 0) {
                printf("error: invalid clk_div value '%u'\n", clk_divider_opt);
                show_usage();
                exit(1);
            }
            break;
        case 'i':
            infile = optarg;
            break;
        case 'a':
            opmode = OPMODE_ANALYZE;
            infile = optarg;
            break;
        case 'k':
            device = optarg;
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

    if ((fdin = open(infile, O_RDONLY)) < 0) {
        errExit("opening input file");
    }

    initSwCrc32Table();

    if (opmode == OPMODE_ANALYZE) {
        if (stat(infile, &st) < 0) {
            errExit("reading input file");
        }

        sig_replay = (uint8_t *) calloc(st.st_size, sizeof(uint8_t));

        rcnt = 0;
        while ((cnt = read(fdin, buf, BUF_SIZE)) > 0) {
            for (c = 0; c < cnt; c++) {
                sig_replay[rcnt] = buf[c];
                rcnt++;
            }
        }

        analyze_replay(sig_replay);

        close(fdin);
        free(sig_replay);
        return 0;
    }
    close(fdin);

    if ((fdout = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
        errExit("opening output file");
    }

    if ((za = zip_open(infile, 0, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        fprintf(stderr, "can't open zip archive `%s': %s/n", infile, buf);
        return 1;
    }

    temp_dir = mkdtemp(temp_template);
    if (temp_dir == NULL) {
        errExit("creating temp dir");
    }
    if (chdir(temp_dir) < 0) {
        errExit("opening temp dir");
    }

    for (i = 0; i < zip_get_num_entries(za, 0); i++) {
        if (zip_stat_index(za, i, 0, &sb) == 0) {
            //printf("==================/n");
            len = strlen(sb.name);
            //printf("Name: [%s], ", sb.name);
            //printf("Size: [%llu], ", sb.size);
            //printf("mtime: [%u]\n", (unsigned int)sb.mtime);
            if (sb.name[len - 1] == '/') {
                // ignore subdirectories
            } else {
                zf = zip_fopen_index(za, i, 0);
                if (zf) {
                    fd = open(sb.name, O_RDWR | O_TRUNC | O_CREAT, 0644);
                    //if (chdir("../") < 0) {
                    //    errExit("opening temp dir");
                    //}
                    if (fd < 0) {
                        fprintf(stderr, "unpacking error for %s file\n", sb.name);
                        continue;
                    }
                    sum = 0;
                    while (sum != sb.size) {
                        len = zip_fread(zf, buf, 100);
                        if (len < 0) {
                            fprintf(stderr, "unpacking error for %s file\n", sb.name);
                            continue;
                        }
                        if (write(fd, buf, len) != len) {
                            errExit("unpacking file");
                        }
                        sum += len;
                    }

                    close(fd);
                    zip_fclose(zf);

                } else {
                    fprintf(stderr, "unpacking error\n");
                    zip_fclose(zf);
                    continue;
                }
            }
        } else {
            printf("File[%s] Line[%d]/n", __FILE__, __LINE__);
        }
    }

    if (zip_close(za) == -1) {
        errExit("can't close zip archive");
    }
    // parse metadata file
    snprintf(temp_file, MAXPATH, "%s/metadata", temp_dir);
    if ((fdin = open(temp_file, O_RDONLY)) < 0) {
        errExit("opening metadata file");
    }
    close(fdin);

    //metadata_t sig_meta;
    s.sig_meta.version = 0;
    s.sig_meta.unitsize = 0;
    s.sig_meta.total_probes = 0;
    s.sig_meta.capturefile = NULL;
    s.sig_meta.samplerate = NULL;

    if (ini_parse(temp_file, metadata_parser, &s.sig_meta) < 0) {
        errExit("cannot parse metadata file");
    }
    // convert human readable samplerate into sampling_interval
    if (strstr(s.sig_meta.samplerate, " MHz")) {
        freq_multiplier = 1E6;
    } else if (strstr(s.sig_meta.samplerate, " kHz")) {
        freq_multiplier = 1E3;
    } else if (strstr(s.sig_meta.samplerate, " Hz")) {
        freq_multiplier = 1;
    } else {
        errExit("unknown input samplerate");
    }

    str_to_uint32((char *)s.sig_meta.samplerate, &freq, 0, strlen(s.sig_meta.samplerate), 0, -1);

    if (freq == 0) {
        fprintf(stderr, "the frequency value read from capture metadata is invalod, exiting\n");
        exit(1);
    }

    s.sampling_interval = (1.0 / (double)freq) / (double)freq_multiplier;
    printf("  input file contains %d channels sampled at %s, sampling interval of %f µs, mask is %x\n",
           s.sig_meta.total_probes, s.sig_meta.samplerate, s.sampling_interval * 1.0E6, mask);

    free((void *)s.sig_meta.samplerate);

    // metadata file parsing has ended

    // parse captured signal file
    //  count the number of chunks
    capture_chunks = 1;
    capture_size_total = 0;
    snprintf(temp_file, MAXPATH, "%s/%s-%u", temp_dir, s.sig_meta.capturefile, capture_chunks);

    while (access(temp_file, F_OK) == 0) {
        capture_chunks++;
        stat(temp_file, &st);
        capture_size_total += st.st_size;
        snprintf(temp_file, MAXPATH, "%s/%s-%u", temp_dir, s.sig_meta.capturefile, capture_chunks);
    }

    capture_chunks--;
    printf("    number of chunks: %u, total size: %u bytes\n", capture_chunks, capture_size_total);

    //  apply mask and shift to capture, send result to memory

    switch (s.sig_meta.unitsize) {
    case 2:
        sig_8ch = (uint8_t *) calloc(capture_size_total / 2, sizeof(uint8_t));
        rcnt = 0;
        for (chunk = 1; chunk <= capture_chunks; chunk++) {
            snprintf(temp_file, MAXPATH, "%s/%s-%u", temp_dir, s.sig_meta.capturefile, chunk);
            if ((fdin = open(temp_file, O_RDONLY)) < 0) {
                errExit("opening signal chunk");
            }
            while ((cnt = read(fdin, buf, BUF_SIZE)) > 0) {
                for (c = 0; c < cnt / 2; c++) {
                    sig16 = *(uint16_t *) (buf + (c * 2));
                    if (lshift) {
                        sig_8ch[rcnt] = (sig16 & mask) << lshift;
                    } else if (rshift) {
                        sig_8ch[rcnt] = (sig16 & mask) >> rshift;
                    } else {
                        sig_8ch[rcnt] = sig16 & mask;
                    }
                    rcnt++;
                }
            }
            close(fdin);
        }

        s.signal_len = capture_size_total / 2;
        s.sig = sig_8ch;
        memset(&replay_header, 0, sizeof(replay_header_t));
        replay_header.clk_divider = clk_divider;
        parse_pulseview(&s, &replay_header, &replay);
        free(sig_8ch);
        break;
    case 1:
        sig_8ch = (uint8_t *) calloc(capture_size_total, sizeof(uint8_t));
        rcnt = 0;
        for (chunk = 1; chunk <= capture_chunks; chunk++) {
            snprintf(temp_file, MAXPATH, "%s/%s-%u", temp_dir, s.sig_meta.capturefile, chunk);
            if ((fdin = open(temp_file, O_RDONLY)) < 0) {
                errExit("opening signal chunk");
            }
            while ((cnt = read(fdin, buf, BUF_SIZE)) > 0) {
                for (c = 0; c < cnt; c++) {
                    if (lshift) {
                        sig_8ch[rcnt] = (buf[c] & (mask & 0xff)) << lshift;
                    } else if (rshift) {
                        sig_8ch[rcnt] = (buf[c] & (mask & 0xff)) >> rshift;
                    } else {
                        sig_8ch[rcnt] = buf[c] & (mask & 0xff);
                    }
                    rcnt++;
                }
            }
            close(fdin);
        }
        s.signal_len = capture_size_total;
        s.sig = sig_8ch;
        memset(&replay_header, 0, sizeof(replay_header_t));
        replay_header.clk_divider = clk_divider;
        parse_pulseview(&s, &replay_header, &replay);
        free(sig_8ch);
        break;
    default:
        fprintf(stderr, "unsuported block size\n");
        exit(1);
        break;
    }

    //printf("%d counts\n", rcnt);

    if (s.sig_meta.capturefile) {
        free((void *)s.sig_meta.capturefile);
    }

    save_replay(fdout, &replay_header, &replay);

    close(fdout);

    recursive_unlink(temp_dir);
}

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static int metadata_parser(void *user, const char *section, const char *name, const char *value)
{
    metadata_t *meta = (metadata_t *) user;

    if (MATCH(device, "total probes")) {
        meta->total_probes = atoi(value);
    } else if (MATCH(device, "unitsize")) {
        meta->unitsize = atoi(value);
    } else if (MATCH(device, "capturefile")) {
        meta->capturefile = strdup(value);
    } else if (MATCH(device, "samplerate")) {
        meta->samplerate = strdup(value);
    } else {
        return 0;
    }
    return 1;
}

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

// CRC32 table - used by both msp430 and lrzsz
void initSwCrc32Table(void)
{
    int i, j;
    uint32_t crc;

    for (i = 0; i < 256; i++) {
        crc = (uint32_t) i;

        for (j = 0; j < 8; j++) {
            if (crc & 0x00000001L) {
                crc = (crc >> 1) ^ POLYNOMIAL_32;
            } else {
                crc = crc >> 1;
            }
        }
        crc32Table[i] = crc;
    }

    // Set flag that the CRC32 table is initialized
    crc32TableInit = 1;
}
