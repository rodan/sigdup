
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
#include "config.h"
#include "tlpi_hdr.h"

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
    int fdin;
    char *infile = NULL;
    char *outfile = NULL;
    int opt;
    char buf[BUF_SIZE];
    int32_t cnt;
    uint32_t c, rcnt = 0;
    uint8_t opmode = OPMODE_NORMAL;
    struct stat st;

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

    if ((fdin = open(infile, O_RDONLY)) < 0) {
        errExit("opening input file");
    }

    if (opmode == OPMODE_ANALYZE) {
        if (stat(infile, &st) < 0) {
            errExit("reading input file");
        }

#if 0
        sig_replay = (uint8_t *) calloc(st.st_size, sizeof(uint8_t));

        rcnt = 0;
        while ((cnt = read(fdin, buf, BUF_SIZE)) > 0) {
            for (c = 0; c < cnt; c++) {
                sig_replay[rcnt] = buf[c];
                rcnt++;
            }
        }

        close(fdin);
        free(sig_replay);
#endif
        return 0;
    }
    close(fdin);

#if 0
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
    printf("  input file contains %d channels sampled at %s, sampling interval of %f µs, mask is 0x%x\n",
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
#endif
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
