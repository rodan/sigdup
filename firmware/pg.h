#ifndef __PG_H_
#define __PG_H_


#define CLK_DIV_1   1
#define CLK_DIV_2   2
#define CLK_DIV_4   4
#define CLK_DIV_8   8
#define CLK_DIV_16  16
#define CLK_DIV_24  24
#define CLK_DIV_32  32
#define CLK_DIV_64  64

typedef struct {
    uint16_t version;
    const char* capturefile;
    uint8_t total_probes;
    const char* samplerate;
    uint8_t unitsize;
} metadata_t;

typedef struct {
    uint8_t version;            /// stream version
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
