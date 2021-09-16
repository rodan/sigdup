#ifndef __PG_H_
#define __PG_H_


typedef struct {
    uint16_t version;
    const char* capturefile;
    uint8_t total_probes;
    const char* samplerate;
    uint8_t unitsize;
} metadata_t;


typedef struct {
    uint8_t version;             /// version
    uint16_t packet_count;       /// number of packets that follow
    uint8_t bytes_per_packet;    /// number of bytes for each packet
    uint8_t block_size;          /// defined as 1 for 8channels, 2 for 16channels
} __attribute__((__packed__)) replay_header_t;

typedef struct {
    uint8_t sig;
    uint16_t clocks_to_next;
} __attribute__((__packed__)) replay_packet_8ch_t;

typedef struct {
    uint16_t sig;
    uint16_t clocks_to_next;
} __attribute__((__packed__)) replay_packet_16ch_t;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
