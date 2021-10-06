#ifndef __PG_H_
#define __PG_H_


typedef struct {
    uint16_t version;
    const char* capturefile;
    uint8_t total_probes;
    const char* samplerate;
    uint8_t unitsize;
} metadata_t;

typedef struct input_sig {
    metadata_t sig_meta;
    ssize_t signal_len;
    uint8_t sample_size;
    double sampling_interval;
    uint8_t *sig;
} input_sig_t;

typedef struct input_edge {
    struct input_edge *next;    /// linked list pointer to next edge_desc_t
    uint32_t c_start;           /// sample count since signal start
    double t_start;             /// microseconds since signal start when this edge was sampled
    uint32_t c_diff_next;       /// sample counts diff until next edge or signal end
    double t_diff_next;         /// microseconds diff until next edge or signal end
    double t_next;              /// time until next edge
    uint16_t sig;               /// actual signal sampled
} input_edge_t;

typedef struct {
    uint8_t version;            /// replay stream version
    uint8_t header_size;        /// replay_header_t size
    uint16_t packet_count;      /// number of packets that follow
    uint8_t bytes_per_packet;   /// number of bytes for each packet
    uint8_t block_size;         /// defined as 1 for 8channels, 2 for 16channels
    uint16_t tactl;             /// TAxCTL (timer control register)
    uint16_t taex0;             /// TAxEX0 (timer expansion 0 register)
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
