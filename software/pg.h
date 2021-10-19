#ifndef __PG_H_
#define __PG_H_

#define      CLK_DIV_1  1
#define      CLK_DIV_2  2
#define      CLK_DIV_3  3
#define      CLK_DIV_4  4
#define      CLK_DIV_5  5
#define      CLK_DIV_6  6
#define      CLK_DIV_7  7
#define      CLK_DIV_8  8
#define     CLK_DIV_10  10
#define     CLK_DIV_12  12
#define     CLK_DIV_14  14
#define     CLK_DIV_16  16
#define     CLK_DIV_20  20
#define     CLK_DIV_24  24
#define     CLK_DIV_28  28
#define     CLK_DIV_32  32
#define     CLK_DIV_40  40
#define     CLK_DIV_48  48
#define     CLK_DIV_56  56
#define     CLK_DIV_64  64

#define    CLK_DIV_CNT  20

#define   BLACKOUT_INT  12e-6 // it takes about 12us to prepare an edge and recover to normal operation

typedef struct {
    uint16_t version;
    const char* capturefile;
    uint8_t total_probes;
    const char* samplerate;
    uint8_t unitsize;
} metadata_t;

typedef struct input_sig {
    metadata_t sig_meta;
    uint32_t signal_len;
    uint8_t sample_size;
    double sampling_interval;   /// time in seconds between ADC samples
    uint8_t *sig;               /// actual input pulseview signal
} input_sig_t;

typedef struct input_edge {
    struct input_edge *next;    /// linked list pointer to next edge_desc_t
    uint32_t c_start;           /// sample count since signal start
    double t_start;             /// microseconds since signal start when this edge was sampled
    uint32_t c_diff_next;       /// sample counts diff until next edge or signal end
    double t_diff_next;         /// microseconds diff until next edge or signal end
    double t_abs;               /// absolute time for the next edge (t0 is at signal start)
    uint16_t sig;               /// actual signal sampled
} input_edge_t;

typedef struct sim_replay {
    double *edge_ref;           /// reference edges from the input signal
    double *edge_sim;           /// simulated edges
    uint32_t edge_cnt;          /// number of edges in input signal
    uint32_t smclk;             /// microcontroller master clock
    uint8_t clk_divider;        /// divider applied to smclk
    double min_tia_err;         /// minimum non-zero time interval error
    double max_tia_err;         /// maximum time interval error
    double avg_tia_err;         /// average time interval error
    double avg_tia_err_res;     /// rescaled 0..100 value for the average time interval error
    double deviation;           /// standard deviation
    uint32_t replay_pkt_cnt;    /// packet count for replay
    double replay_pkt_cnt_res;  /// rescaled 0..100 value for the packet count
    uint32_t blackout_err;      /// counter for transitions that happen within the uc processing timeframe
    double score;
} sim_replay_t;

typedef struct {
    uint8_t version;            /// replay stream version
    uint8_t header_size;        /// replay_header_t size
    uint16_t packet_count;      /// number of packets that follow
    uint8_t bytes_per_packet;   /// number of bytes for each packet
    uint8_t block_size;         /// defined as 1 for 8channels, 2 for 16channels
    uint8_t clk_divider;        /// timer divider
    uint16_t data_checksum;     /// crc16 hash for all the data packets
    uint16_t header_checksum;   /// crc16 hash for replay_header_t
} __attribute__((__packed__)) replay_header_t;

typedef struct {
    uint8_t sig;                /// P1, P2 output
    uint16_t ccr;               /// compare timer register
} __attribute__((__packed__)) replay_packet_8ch_t;

typedef struct replay_ll {
    struct replay_ll *next;
    void *pkt;
} replay_ll_t;

typedef struct {
    uint16_t sig;               /// P1 output
    uint16_t ccr;               /// compare timer register
} __attribute__((__packed__)) replay_packet_16ch_t;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
