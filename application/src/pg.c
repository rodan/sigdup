
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "helper.h"
#include "tlpi_hdr.h"

#define          BUF_SIZE  1024000
#define  BLOCK_SIZE_1BYTE  1
#define BLOCK_SIZE_2BYTES  2

int main(int argc, char *argv[])
{
    int fdin, fdout;
    char *infile = NULL;
    char *outfile = NULL;
    int opt;
    char buf[BUF_SIZE];
    ssize_t cnt;
    //ssize_t seek_cnt = 0;
    uint64_t seq_cnt = 1;
    uint64_t i;
    uint16_t ccl; // current adc conversion level
    uint8_t oll; // old logic level
    uint8_t cll; // current logic level
    uint64_t test = 1;
    uint8_t channel;
    uint8_t block_size = BLOCK_SIZE_2BYTES;

    while ((opt = getopt(argc, argv, "b:i:o:c:")) != -1) {
        switch (opt) {
        case 'b':
            hstr_to_uint8(optarg, &block_size, 0, strlen(optarg) - 1, 1, 2);
            break;
        case 'c':
            hstr_to_uint8(optarg, &channel, 0, strlen(optarg) - 1, 0, 16);
            break;
        case 'i':
            infile = optarg;
            break;
        case 'o':
            outfile = optarg;
            break;
        default:
            break;
        }
    }

    if ((fdin = open(infile, O_RDONLY)) < 0) {
        errExit("opening input file");
    }

    if ((fdout = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
        errExit("opening output file");
    }

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

                for (i=0;i<cnt;i+=2) {
                    // get current sample point
                    ccl = (buf[i] + (buf[i+1] << 8)) & channel;
                    if (ccl) {
                        cll = 1;
                        test++;
                    } else {
                        cll = 0;
                        test++;
                    }
    
                    if (cll != oll) {
                        printf ("%d %lu\r\n", oll, seq_cnt);
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

                for (i=0;i<cnt;i++) {
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
                        printf ("%d %lu\r\n", oll, seq_cnt);
                        oll = cll;
                        seq_cnt = 1;
                    } else {
                        seq_cnt++;
                    }
                }
            }
            break;
        }

    printf ("%d %lu\r\n", cll, seq_cnt);

    close(fdin);
    close(fdout);

    printf("test is %lu\r\n", test);
}


