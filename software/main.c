
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <fts.h>
#include <zip.h>

#include "version.h"
#include "lib_convert.h"
#include "tlpi_hdr.h"
#include "ini.h"
#include "zcrc.h"
#include "list.h"
#include "pg.h"

#define          BUF_SIZE  1024000
#define  BLOCK_SIZE_1BYTE  1
#define BLOCK_SIZE_2BYTES  2

#define           MAXPATH  256
#define        DEF_DEVICE  "device 1"

static int metadata_parser(void *user, const char *section, const char *name, const char *value);
static void create_dir(const char *dir);
static int recursive_unlink(const char *dir);

char *device;                   /// device name as used in the metadata file

static uint8_t block_size = BLOCK_SIZE_1BYTE;   /// block size (in bytes) of the input capture
static uint16_t mask = 0xffff;  /// mask to be applied to the captured signal - set bitmask 1 to channels that are of interest. default 0xffff
static uint8_t shift = 0;       /// left shift input signal by these many bits, so that a channel of interest lands on a particular pin on ports P1/P2

#define     POLYNOMIAL_32  0xEDB88320
#define             SMCLK  16000000


// Holds a crc32 lookup table
uint32_t crc32Table[256];
uint8_t crc32TableInit = 0;
// crc32 used in the zcrc library
void initSwCrc32Table(void);
void show_usage(void);
void show_version(void);
static int parse_signal(input_sig_t * s, replay_header_t * hdr, void **replay);
static int save_replay(int fd, replay_header_t * hdr, void **replay);

LIST(replay);

void show_usage(void)
{
    printf("Usage: pg [OPTION]...\n");
    printf
        ("Convert PulseView capture into a highly-compressed file used to replay the signals\n\n");
    printf("mandatory options:\n");
    printf(" -i [FILE]   input PulseView '.sr' file\n");
    printf(" -o [FILE]   output converted file\n");
    printf("\n");
    printf("non-mandatory options:\n");
    printf(" -b [NUM]    block size (in bytes) of the input capture, default %d byte(s)\n",
           block_size);
    printf(" -m [NUM]    mask (in hex) to be applied to the input port, default 0x%x\n", mask);
    printf(" -s [NUM]    number of bits the output signal is shifted to the left, default %d\n",
           shift);
    printf("  --help    display this help and exit\n");
    printf("  --version output version information and exit\n");

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
    ssize_t cnt, c, rcnt = 0;
    uint64_t i;

    // zip related
    struct zip *za;
    struct zip_file *zf;
    struct zip_stat sb;
    int err;
    int len;
    int fd;
    long long sum;
    char temp_path[MAXPATH];
    uint32_t freq_multiplier = 0;
    uint32_t freq = 0;

    struct stat st;

    uint8_t *sig_8ch;
    //uint16_t *sig_16ch;
    //uint16_t last_16ch;

    replay_header_t replay_header;
    list_init(replay);

    input_sig_t s;

    device = DEF_DEVICE;

    while ((opt = getopt(argc, argv, "hvb:i:o:d:m:s:")) != -1) {
        switch (opt) {
        case 'b':
            hstr_to_uint8(optarg, &block_size, 0, strlen(optarg) - 1, 1, 2);
            break;
        case 'm':
            hstr_to_uint16(optarg, &mask, 0, strlen(optarg) - 1, 0, -1);
            break;
        case 's':
            hstr_to_uint8(optarg, &shift, 0, strlen(optarg) - 1, 0, -1);
            break;
        case 'i':
            infile = optarg;
            break;
        case 'd':
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

    if ((infile == NULL) || (outfile == NULL)) {
        printf("Error: provide input and output files\n");
        show_usage();
        exit(1);
    }

    initSwCrc32Table();

    if ((fdin = open(infile, O_RDONLY)) < 0) {
        errExit("opening input file");
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
                    snprintf(temp_path, MAXPATH, ".%s", infile);
                    create_dir(temp_path);
                    if (chdir(temp_path) < 0) {
                        errExit("opening temp dir");
                    }
                    fd = open(sb.name, O_RDWR | O_TRUNC | O_CREAT, 0644);
                    if (chdir("../") < 0) {
                        errExit("opening temp dir");
                    }
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
    snprintf(temp_path, MAXPATH, ".%s/metadata", infile);
    if ((fdin = open(temp_path, O_RDONLY)) < 0) {
        errExit("opening metadata file");
    }
    close(fdin);

    //metadata_t sig_meta;
    s.sig_meta.version = 0;
    s.sig_meta.unitsize = 0;
    s.sig_meta.total_probes = 0;
    s.sig_meta.capturefile = NULL;
    s.sig_meta.samplerate = NULL;

    if (ini_parse(temp_path, metadata_parser, &s.sig_meta) < 0) {
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

    s.sampling_interval = (1.0 / (double)freq) / (double)freq_multiplier;
    printf("%d channels sampled at %s, sampling interval of %f µs, mask is %x\n",
           s.sig_meta.total_probes, s.sig_meta.samplerate, s.sampling_interval * 1.0E6, mask);

    if (s.sig_meta.samplerate) {
        free((void *)s.sig_meta.samplerate);
    }
    // metadata file parsing has ended

    // parse captured signal file
    snprintf(temp_path, MAXPATH, ".%s/%s-1", infile, s.sig_meta.capturefile);
    if ((fdin = open(temp_path, O_RDONLY)) < 0) {
        errExit("opening signal file");
    }

    if (s.sig_meta.capturefile) {
        free((void *)s.sig_meta.capturefile);
    }
    // apply mask and shift to capture, send result to memory
    stat(temp_path, &st);

    switch (block_size) {
    case BLOCK_SIZE_2BYTES:
        //parse_16bit_sig();
        break;
    case BLOCK_SIZE_1BYTE:

        sig_8ch = (uint8_t *) calloc(st.st_size, sizeof(uint8_t));
        rcnt = 0;
        while ((cnt = read(fdin, buf, BUF_SIZE)) > 0) {
            for (c = 0; c < cnt; c++) {
                sig_8ch[rcnt] = (buf[c] & (mask & 0xff)) << shift;
                //printf("%c", sig_8ch[rcnt]);
                rcnt++;
            }
        }
        s.signal_len = st.st_size;
        s.sig = sig_8ch;
        parse_signal(&s, &replay_header, NULL);
        free(sig_8ch);
        break;
    default:
        errExit("unsuported block size");
        break;
    }

    //printf("%d counts\n", rcnt);

    save_replay(fdout, &replay_header, NULL);

    close(fdin);
    close(fdout);

    snprintf(temp_path, MAXPATH, ".%s", infile);
    recursive_unlink(temp_path);
}

static int parse_signal(input_sig_t * s, replay_header_t * hdr, void **foo)
{
    uint8_t last_8ch;
    uint32_t parsed_cnt = 0;    /// parsed samples count
    ssize_t pktcnt = 0;         /// replay data packet count
    ssize_t ucnt;               /// how many clocks the signal remains unchanged
    ssize_t c;
    uint32_t timer_ticks;
    //uint8_t timer_divider;

    double si = 1.0E99;         /// smallest interval between two edges

    input_edge_t *edge;
    replay_packet_8ch_t *rp;
    replay_ll_t *re;

    replay_packet_8ch_t replay_packet_8ch;
    // parse sig_8ch and convert into replay stream
    memset(hdr, 0, sizeof(replay_header_t));

    LIST(input_edges);
    list_init(input_edges);

    pktcnt = 0;
    parsed_cnt = 0;
    hdr->data_checksum = 0;

    for (c = 0; c < s->signal_len; c++) {
        last_8ch = s->sig[c];

        // see for how many samples this signal remains unchanged
        for (ucnt = 1; ucnt < s->signal_len - c; ucnt++) {
            if (s->sig[c + ucnt] != last_8ch) {
                break;
            }
        }

        // add new edge to the linked list
        edge = (input_edge_t *) calloc(1, sizeof(input_edge_t));

        if (si > ucnt * s->sampling_interval) {
            si = ucnt * s->sampling_interval;
        }

        //edge->c_start = ;
        //edge->t_start = ;
        edge->c_diff_next = ucnt;
        edge->t_diff_next = ucnt * s->sampling_interval;
        edge->t_next = ucnt * s->sampling_interval + parsed_cnt * s->sampling_interval;
        edge->sig = last_8ch;
        //edge->ccr = ;

        list_add(input_edges, edge);

        parsed_cnt += ucnt;
        c += ucnt - 1;

        pktcnt++;
    }

    hdr->version = 1;
    hdr->packet_count = pktcnt;
    hdr->bytes_per_packet = sizeof(replay_packet_8ch);
    hdr->block_size = sizeof(replay_packet_8ch.sig);
    hdr->header_size = sizeof(replay_header_t);
    hdr->header_checksum = zcrc16(hdr, sizeof(replay_header_t) - 2, 0);

    printf("smallest interval is %f\r\n", si);

    // calculate uc timer registers

    // 1 timer tick is 1us
    //hdr->tactl = ;// TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // SMCLK divided by 8
    hdr->taex0 = 0x1; //TAIDEX_1; // further divide by 2
    //timer_divider = 16;

    input_edge_t *edgep;
    for (edgep = list_head(input_edges); edgep != NULL; edgep = edgep->next) {

        // add replay packet
        rp = (replay_packet_8ch_t *) calloc(1, sizeof(replay_packet_8ch_t));
        re = (replay_ll_t *) calloc(1, sizeof(replay_ll_t));
        re->pkt = rp;

        timer_ticks = edgep->t_next * 1.0E6;

        rp->ccr = timer_ticks; // FIXME check for 2^16 overflow
        rp->sig = edgep->sig;
        list_add(replay, re);

        printf("diff_next %fs, %uc, t_next %f,sig %x\r\n", edgep->t_diff_next, edgep->c_diff_next, edgep->t_next, edgep->sig);
    }

    return EXIT_SUCCESS;
}

static int save_replay(int fd, replay_header_t * hdr, void **foo)
{
    replay_ll_t *a;
    replay_packet_8ch_t *p;

    // output an empty header, to be filled after we have the data itself
    if (write(fd, hdr, sizeof(replay_header_t)) != sizeof(replay_header_t)) {
        errExit("during write");
    }

    // output the data
    for (a = *replay; a != NULL; a = a->next) {
        p = (replay_packet_8ch_t *) a->pkt;

        if (write(fd, p, sizeof(replay_packet_8ch_t)) != sizeof(replay_packet_8ch_t) ) {
            errExit("during write");
        }

        hdr->data_checksum =
            zcrc16(p, sizeof(replay_packet_8ch_t), hdr->data_checksum);

        printf(" sig 0x%04x  ccr %u\r\n", p->sig, p->ccr);
    }

    // output final header
    if (pwrite(fd, hdr, sizeof(replay_header_t), 0) != sizeof(replay_header_t)) {
        errExit("during pwrite");
    }

    return EXIT_SUCCESS;
}

#if 0
static int parse_16bit_sig()
{

    // initialize readings
    switch (block_size) {
    case BLOCK_SIZE_2BYTES:
        read(fdin, buf, BLOCK_SIZE_2BYTES);
        ccl = (buf[0] + (buf[1] << 8)) & channel;
        if (ccl) {
            oll = 1;
        } else {
            oll = 0;
        }
        break;

    case BLOCK_SIZE_1BYTE:
        read(fdin, buf, BLOCK_SIZE_1BYTE);
        ccl = buf[0] & channel;
        if (ccl) {
            oll = 1;
        } else {
            oll = 0;
        }
        break;
    default:
        errExit("unsuported block size");
        break;
    }

    // read all values
    switch (block_size) {

    case BLOCK_SIZE_2BYTES:
        while ((cnt = read(fdin, buf, BUF_SIZE)) > 0) {

            for (i = 0; i < cnt; i += 2) {
                // get current sample point
                ccl = (buf[i] + (buf[i + 1] << 8)) & channel;
                if (ccl) {
                    cll = 1;
                    test++;
                } else {
                    cll = 0;
                    test++;
                }

                if (cll != oll) {
                    printf("%d %lu\r\n", oll, seq_cnt);
                    oll = cll;
                    seq_cnt = 1;
                } else {
                    seq_cnt++;
                }
            }
        }
        break;
    case BLOCK_SIZE_1BYTE:
        while ((cnt = read(fdin, buf, BUF_SIZE)) > 0) {

            for (i = 0; i < cnt; i++) {
                // get current sample point
                ccl = buf[i] & channel;
                if (ccl) {
                    cll = 1;
                    test++;
                } else {
                    cll = 0;
                    test++;
                }

                if (cll != oll) {
                    printf("%d %lu\r\n", oll, seq_cnt);
                    oll = cll;
                    seq_cnt = 1;
                } else {
                    seq_cnt++;
                }
            }
        }
        break;
    }

    printf("%d %lu\r\n", cll, seq_cnt);
    return EXIT_SUCCESS;
}
#endif

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

static void create_dir(const char *dir)
{
    if (mkdir(dir, 0755) < 0) {
        if (errno != EEXIST) {
            errExit("mkdir failed");
        }
    }
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
            fprintf(stderr, "%s: fts_read error: %s\n",
                    curr->fts_accpath, strerror(curr->fts_errno));
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
                fprintf(stderr, "%s: Failed to remove: %s\n",
                        curr->fts_path, strerror(curr->fts_errno));
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
