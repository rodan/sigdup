
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "config.h"
#include "pg.h"
#include "list.h"
#include "zcrc.h"
#include "tlpi_hdr.h"
#include "sig_mng.h"

LIST(input_edges);
extern const uint8_t clk_dividers[];

int8_t parse_pulseview(input_sig_t * s, replay_header_t * hdr, list_t * replay_ll)
{
    uint8_t last_8ch;
    uint32_t parsed_cnt = 0;    /// parsed samples count
    uint32_t edge_cnt = 0;      /// number of edges in the pulseview signal
    uint32_t ucnt;              /// how many clocks the signal remains unchanged
    uint32_t c;
    double *edge_ref;
    sim_replay_t sim[CLK_DIV_CNT];
    double avg_tia_max = 0;
    uint32_t replay_pkt_cnt_max = 0;
    double score_max = 0;
    uint8_t clk_divider_forced = hdr->clk_divider; /// 0 if the divider should be detected automatically
    uint8_t clk_divider_optimal = 1;
    uint8_t impossible_to_replay = 1;

    input_edge_t *edge;
    input_edge_t *edged;
    input_edge_t *edgep;

    replay_packet_8ch_t replay_packet_8ch;

    list_init(input_edges);

    edge_cnt = 0;
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

        edge->c_diff_next = ucnt;
        edge->t_diff_next = ucnt * s->sampling_interval;
        edge->t_abs = (ucnt + parsed_cnt) * s->sampling_interval;
        edge->sig = last_8ch;

        list_add(input_edges, edge);

        parsed_cnt += ucnt;
        c += ucnt - 1;

        edge_cnt++;
    }

    //printf("list has %u edges\n", edge_cnt);

    // fill an array with the edges present in the input signal
    edge_ref = (double *)calloc(edge_cnt, sizeof(double));

    // run simulations with different timer dividers

    c = 0;
    for (edgep = list_head(input_edges); edgep != NULL; edgep = edgep->next) {
        edge_ref[c] = edgep->t_abs;
        c++;
    }

    printf("  div\t   min error \t   max error \t   avg err \t       blk     pkts        score  \n");

    for (c = 0; c < CLK_DIV_CNT; c++) {
        sim[c].edge_ref = edge_ref;
        sim[c].edge_cnt = edge_cnt;
        sim[c].smclk = SMCLK;
        sim[c].clk_divider = clk_dividers[c];
        simulate_replay(&sim[c]);

        // get the packet count for every simulation
        hdr->clk_divider = clk_dividers[c];
        hdr->packet_count = 0;
        if (sim[c].blackout_err == 0) {
            impossible_to_replay = 0;
            generate_replay(hdr, replay_ll, DEFAULTS);
        }
        sim[c].replay_pkt_cnt = hdr->packet_count;

        if (sim[c].avg_tia_err > avg_tia_max) {
            avg_tia_max = sim[c].avg_tia_err;
        }

        if (sim[c].replay_pkt_cnt > replay_pkt_cnt_max) {
            replay_pkt_cnt_max = sim[c].replay_pkt_cnt;
        }
    }

    // rescale values in order to calculate a score
    for (c = 0; c < CLK_DIV_CNT; c++) {
        if (avg_tia_max) {
            sim[c].avg_tia_err_res = sim[c].avg_tia_err * 100.0 / avg_tia_max + 1;
        } else {
            sim[c].avg_tia_err_res = 1;
        }

        if (replay_pkt_cnt_max) {
            sim[c].replay_pkt_cnt_res = sim[c].replay_pkt_cnt * 100.0 / replay_pkt_cnt_max;
        } else {
            // if replay_pkt_cnt_max is zero then blackout_err has been trigerred for EVERY div simulation
            // which is very bad
            sim[c].replay_pkt_cnt_res = 100;
        }

        sim[c].score = 50.0 / sim[c].avg_tia_err_res + 49.0 / sim[c].replay_pkt_cnt_res + 1.0 / sim[c].clk_divider;

        // pick the best clk_divider
        if (sim[c].score > score_max) {
            score_max = sim[c].score;
            clk_divider_optimal = sim[c].clk_divider;
        }

        printf("    %u\t    %.4fus\t    %.4fus\t    %.4fus\t    %u\t    %u\t    %.0f\n",
               sim[c].clk_divider, sim[c].min_tia_err * 1e6, sim[c].max_tia_err * 1e6, sim[c].avg_tia_err * 1e6,
               sim[c].blackout_err, sim[c].replay_pkt_cnt, sim[c].score);
    }
    free(edge_ref);

    if (impossible_to_replay) {
        printf("input signal has transitions that can't be replayed on the target microcontroller, exiting\n");
        edgep = list_head(input_edges);
        while (edgep != NULL) {
            edged = edgep;
            edgep = edgep->next;
            free(edged);
        }
        return EXIT_FAILURE;
    }

    // generate replay stream with the best divider
    if (clk_divider_forced) {
        hdr->clk_divider = clk_divider_forced;
        printf("  optimal divider: %u, generating using forced divider: %u\n", clk_divider_optimal, clk_divider_forced);
    } else {
        hdr->clk_divider = clk_divider_optimal;
        printf("  optimal divider: %u\n", clk_divider_optimal);
    }
    hdr->version = 1;
    hdr->bytes_per_packet = sizeof(replay_packet_8ch);
    hdr->block_size = sizeof(replay_packet_8ch.sig);
    hdr->header_size = sizeof(replay_header_t);

    generate_replay(hdr, replay_ll, ALLOC | VERBOSE);

    // free input signal's linked list
    edgep = list_head(input_edges);
    while (edgep != NULL) {
        edged = edgep;
        edgep = edgep->next;
        free(edged);
    }

    return EXIT_SUCCESS;
}

void simulate_replay(sim_replay_t * sim)
{
    uint32_t c;
    uint32_t timer_ticks;
    double rsampling_int;
    double edge_err;
    double total_err = 0;
    double last_edge = 0;       /// timestamp of the last edge

    rsampling_int = 1.0 / (sim->smclk / sim->clk_divider);
    sim->min_tia_err = 0;
    sim->max_tia_err = 0;
    sim->blackout_err = 0;
    sim->edge_sim = (double *)calloc(sim->edge_cnt, sizeof(double));

    //printf("rsampling %f us (1/ (%u / %u) Hz) \r\n", rsampling_int * 1e6, sim->smclk, sim->clk_divider);

    for (c = 0; c < sim->edge_cnt; c++) {
        timer_ticks = sim->edge_ref[c] / rsampling_int;
        sim->edge_sim[c] = timer_ticks * rsampling_int;

        if (sim->edge_sim[c] - last_edge < BLACKOUT_INT) {
            sim->blackout_err++;
        }

        edge_err = sim->edge_ref[c] - sim->edge_sim[c];
        if (edge_err > sim->max_tia_err) {
            sim->max_tia_err = edge_err;
            if (sim->min_tia_err == 0) {
                sim->min_tia_err = sim->max_tia_err;
            }
        }

        if ((edge_err > 0) && (edge_err < sim->min_tia_err)) {
            sim->min_tia_err = edge_err;
        }

        total_err += edge_err;
        last_edge = sim->edge_ref[c];

        //printf("edge_sim ref %f sim %f diff %f\r\n", sim->edge_ref[c] * 1e6, sim->edge_sim[c] * 1e6, edge_err * 1e6);
    }

    sim->avg_tia_err = total_err / sim->edge_cnt;

    free(sim->edge_sim);
}

int8_t generate_replay(replay_header_t * hdr, list_t * replay_ll, const uint8_t flags)
{
    uint32_t timer_ticks;
    uint32_t timer_ticks_remain;
    double rsampling_int;

    input_edge_t *edgep;
    replay_packet_8ch_t *rp;
    replay_ll_t *re;

    rsampling_int = 1.0 / (SMCLK / hdr->clk_divider);

    if (flags & VERBOSE) {
        printf("replay: sampling interval is %f µs ((%u / %u) Hz)\n", rsampling_int * 1.0E6, SMCLK,
               hdr->clk_divider);
    }

    hdr->packet_count = 0;

    // calculate uc timer registers
    for (edgep = list_head(input_edges); edgep != NULL; edgep = edgep->next) {

        timer_ticks = edgep->t_abs / rsampling_int;

        timer_ticks_remain = timer_ticks;
        while (timer_ticks_remain) {
            if (timer_ticks_remain > 65500) {
                timer_ticks = 65500;
            } else {
                timer_ticks = timer_ticks_remain;
            }

            if (flags & ALLOC) {
                // add replay packet
                rp = (replay_packet_8ch_t *) calloc(1, sizeof(replay_packet_8ch_t));
                re = (replay_ll_t *) calloc(1, sizeof(replay_ll_t));
                re->pkt = rp;

                rp->ccr = timer_ticks;
                rp->sig = edgep->sig;
                list_add(*replay_ll, re);
            }

            hdr->packet_count++;
            timer_ticks_remain -= timer_ticks;

            if (flags & VERBOSE) {
                printf("diff_next %fs, %uc, t_abs %f,sig %x\n", edgep->t_diff_next,
                       edgep->c_diff_next, edgep->t_abs, edgep->sig);
            }
        }
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
    hdr->packet_count = 0;
    for (a = (replay_ll_t *) ** replay_ll; a != NULL; a = a->next) {
        p = (replay_packet_8ch_t *) a->pkt;

        if (write(fd, p, sizeof(replay_packet_8ch_t)) != sizeof(replay_packet_8ch_t)) {
            errExit("during write");
        }

        hdr->data_checksum = zcrc16(p, sizeof(replay_packet_8ch_t), hdr->data_checksum);

        printf(" sig 0x%04x  ccr %u\n", p->sig, p->ccr);
        hdr->packet_count++;
    }

    hdr->header_checksum = zcrc16(hdr, sizeof(replay_header_t) - 2, 0);

    // output final header
    if (pwrite(fd, hdr, sizeof(replay_header_t), 0) != sizeof(replay_header_t)) {
        errExit("during pwrite");
    }
    // free replay signal's linked list
    a = (replay_ll_t *) ** replay_ll;
    while (a != NULL) {
        ad = a;
        pd = (replay_packet_8ch_t *) a->pkt;
        a = a->next;
        free(pd);
        free(ad);
    }

    return EXIT_SUCCESS;
}

void analyze_replay(uint8_t * buf)
{
    uint8_t *stream_pos;        /// address for the next stream data pkt
    uint8_t *stream_start;      /// address where the stream starts
    uint8_t *stream_end;        /// address where the stream ends

    uint8_t next_sig = 0;
    uint16_t next_ccr = 0;

    replay_header_t *hdr;
    hdr = (replay_header_t *) buf;

    printf(" header\n");
    printf("             version  %u\n", hdr->version);
    printf("         header_size  %u\n", hdr->header_size);
    printf("        packet_count  %u\n", hdr->packet_count);
    printf("    bytes_per_packet  %u\n", hdr->bytes_per_packet);
    printf("          block_size  %u\n", hdr->block_size);
    printf("         clk_divider  %u\n", hdr->clk_divider);
    printf("       data_checksum  0x%04x\n", hdr->data_checksum);
    printf("     header_checksum  0x%04x\n", hdr->header_checksum);
    printf("\n");

    stream_start = (uint8_t *) (buf + hdr->header_size);
    stream_end = (uint8_t *) (buf + hdr->header_size + (hdr->packet_count * 3));
    stream_pos = stream_start;

    while (stream_pos < stream_end) {
        next_sig = *((uint8_t *) stream_pos);
        next_ccr = *((uint16_t *) (stream_pos + 1));
        printf(" sig 0x%04x  ccr %u\n", next_sig, next_ccr);
        stream_pos += 3;
    }
}
