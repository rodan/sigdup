
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "pg.h"
#include "list.h"
#include "zcrc.h"
#include "tlpi_hdr.h"
#include "mng.h"

int8_t parse_signal(input_sig_t * s, replay_header_t * hdr, list_t * replay_ll)
{
    uint8_t last_8ch;
    uint32_t parsed_cnt = 0;    /// parsed samples count
    uint32_t pktcnt = 0;        /// replay data packet count
    uint32_t ucnt;              /// how many clocks the signal remains unchanged
    uint32_t c;
    uint32_t timer_ticks;
    uint32_t timer_ticks_remain;
    double rsampling_int;
    uint8_t clk_divider;

    double si = 1.0E99;         /// smallest interval between two edges

    input_edge_t *edge;
    input_edge_t *edged;
    replay_packet_8ch_t *rp;
    replay_ll_t *re;

    replay_packet_8ch_t replay_packet_8ch;
    // parse sig_8ch and convert into replay stream
    //memset(hdr, 0, sizeof(replay_header_t));

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
        edge->c_diff_next = ucnt;
        edge->t_diff_next = ucnt * s->sampling_interval;
        edge->t_next = (ucnt + parsed_cnt) * s->sampling_interval;
        edge->sig = last_8ch;

        list_add(input_edges, edge);

        parsed_cnt += ucnt;
        c += ucnt - 1;

        pktcnt++;
    }

    clk_divider = hdr->clk_divider;

    hdr->version = 1;
    hdr->packet_count = pktcnt;
    hdr->bytes_per_packet = sizeof(replay_packet_8ch);
    hdr->block_size = sizeof(replay_packet_8ch.sig);
    hdr->header_size = sizeof(replay_header_t);
    hdr->header_checksum = zcrc16(hdr, sizeof(replay_header_t) - 2, 0);
    rsampling_int = 1.0 / (SMCLK / clk_divider);

    printf("replay: smallest interval is %f, sampling interval is %f µs ((%u / %u) Hz)\r\n", si,
           rsampling_int * 1.0E6, SMCLK, clk_divider);

    // calculate uc timer registers

    //hdr->clk_divider = clk_divider;

    input_edge_t *edgep;
    for (edgep = list_head(input_edges); edgep != NULL; edgep = edgep->next) {

        timer_ticks = edgep->t_next / rsampling_int;

        timer_ticks_remain = timer_ticks;
        while (timer_ticks_remain) {
            if (timer_ticks_remain > 65500) {
                timer_ticks = 65500;
            } else {
                timer_ticks = timer_ticks_remain;
            }

            // add replay packet
            rp = (replay_packet_8ch_t *) calloc(1, sizeof(replay_packet_8ch_t));
            re = (replay_ll_t *) calloc(1, sizeof(replay_ll_t));
            re->pkt = rp;

            rp->ccr = timer_ticks;      // FIXME check for 2^16 overflow
            rp->sig = edgep->sig;
            list_add(*replay_ll, re);
            timer_ticks_remain -= timer_ticks;

            printf("diff_next %fs, %uc, t_next %f,sig %x\r\n", edgep->t_diff_next,
                   edgep->c_diff_next, edgep->t_next, edgep->sig);
        }
    }

    // free input signal's linked list
    edgep = list_head(input_edges);
    while (edgep != NULL) {
        edged = edgep;
        edgep = edgep->next;
        free(edged);
    }

    return EXIT_SUCCESS;
}

int8_t save_replay(int fd, replay_header_t * hdr, list_t * replay_ll)
{
    replay_ll_t *a;
    replay_ll_t *ad;
    replay_packet_8ch_t *p;
    replay_packet_8ch_t *pd;

    // output an empty header, to be filled after we have the data itself
    if (write(fd, hdr, sizeof(replay_header_t)) != sizeof(replay_header_t)) {
        errExit("during write");
    }
    // output the data
    for (a = (replay_ll_t *) **replay_ll; a != NULL; a = a->next) {
        p = (replay_packet_8ch_t *) a->pkt;

        if (write(fd, p, sizeof(replay_packet_8ch_t)) != sizeof(replay_packet_8ch_t)) {
            errExit("during write");
        }

        hdr->data_checksum = zcrc16(p, sizeof(replay_packet_8ch_t), hdr->data_checksum);

        printf(" sig 0x%04x  ccr %u\r\n", p->sig, p->ccr);
    }

    // output final header
    if (pwrite(fd, hdr, sizeof(replay_header_t), 0) != sizeof(replay_header_t)) {
        errExit("during pwrite");
    }
    // free replay signal's linked list
    a = (replay_ll_t *) **replay_ll;
    while (a != NULL) {
        ad = a;
        pd = (replay_packet_8ch_t *) a->pkt;
        a = a->next;
        free(pd);
        free(ad);
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

void analyze_replay(uint8_t * buf)
{
    uint8_t *stream_pos;        /// address for the next stream data pkt
    uint8_t *stream_start;      /// address where the stream starts
    uint8_t *stream_end;        /// address where the stream ends

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
    printf("         clk_divider  %u\r\n", hdr->clk_divider);
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
