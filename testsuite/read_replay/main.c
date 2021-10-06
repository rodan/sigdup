
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
#include "zcrc.h"
#include "pg.h"

#define     POLYNOMIAL_32  0xEDB88320

// Holds a crc32 lookup table
uint32_t crc32Table[256];
uint8_t crc32TableInit = 0;
// crc32 used in the zcrc library
void initSwCrc32Table(void);
void show_usage(void);
void show_version(void);
void parse_replay(uint8_t *buf);

void show_usage(void)
{
    printf("Usage: pg_read [OPTION]...\n");
    printf
        ("Convert PulseView capture into a highly-compressed file used to replay the signals\n\n");
    printf("mandatory options:\n");
    printf(" -i [FILE]   input replay file\n");
    printf("\n");
    printf("non-mandatory options:\n");
    printf("  --help    display this help and exit\n");
    printf("  --version output version information and exit\n");

}

void show_version(void)
{
    printf("pg_read %d.%d\nbuild %d commit %d\n", VER_MAJOR, VER_MINOR, BUILD, COMMIT);
}

int main(int argc, char *argv[])
{
    int fdin;
    int opt;
    char *infile = NULL;

    uint8_t *sig_replay;
    struct stat st;

    while ((opt = getopt(argc, argv, "hvi:")) != -1) {
        switch (opt) {
        case 'i':
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
        default:
            break;
        }
    }

    if (infile == NULL) {
        printf("Error: provide input file\n");
        show_usage();
        exit(1);
    }

    initSwCrc32Table();

    if ((fdin = open(infile, O_RDONLY)) < 0) {
        errExit("opening input file");
    }

    stat(infile, &st);

    sig_replay = (uint8_t *) calloc(st.st_size, sizeof(uint8_t));

    if (read(fdin, sig_replay, st.st_size) < 0) {
        errExit("reading input file");
    }

    parse_replay(sig_replay);

    close(fdin);
}

void parse_replay(uint8_t *buf)
{
    uint8_t *stream_pos;   /// address for the next stream data pkt
    uint8_t *stream_start; /// address where the stream starts
    uint8_t *stream_end;   /// address where the stream ends

    uint8_t next_sig = 0;
    uint16_t next_ccr = 0;

    replay_header_t *hdr;
    hdr = (replay_header_t *) buf;

    printf(" header\r\n");
    printf("             version  %u\r\n", hdr->version);
    printf("         header_size  %u\r\n", hdr->header_size);
    printf("        packet_count  %u\r\n", hdr->packet_count);
    printf("    bytes_per_packet  %u\r\n", hdr->bytes_per_packet);
    printf("          block_size  %u\r\n", hdr->block_size);
    printf("               tactl  0x%04x\r\n", hdr->tactl);
    printf("               taex0  0x%04x\r\n", hdr->taex0);
    printf("       data_checksum  0x%04x\r\n", hdr->data_checksum);
    printf("     header_checksum  0x%04x\r\n", hdr->header_checksum);
    printf("\r\n");

    stream_start = (uint8_t *) (buf + hdr->header_size);
    stream_end = (uint8_t *) (buf + hdr->header_size + (hdr->packet_count * 3));
    stream_pos = stream_start;

    while (stream_pos < stream_end) {
        next_sig = *((uint8_t *) stream_pos);
        next_ccr = *((uint16_t *) (stream_pos + 1));
        printf(" sig 0x%04x  ccr %u\r\n", next_sig, next_ccr);
        stream_pos += 3;
    }
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
