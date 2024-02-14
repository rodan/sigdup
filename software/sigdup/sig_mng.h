
//  author:          Petre Rodan <2b4eda@subdimension.ro>
//  available from:  https://github.com/rodan/sigdup
//  license:         GNU GPLv3

#ifndef __SIG_MNG_H__
#define __SIG_MNG_H__

#define    DEFAULTS  0x0    /// don't allocate replay linked list, absolute measurements
#define       ALLOC  0x1    /// actually alocate replay linked list
#define  REL_REPLAY  0x2    /// calculate replay based on relative measurements between consecutive transitions
#define     VERBOSE  0x4

#ifdef __cplusplus
extern "C" {
#endif

int8_t parse_pulseview(input_sig_t * s, replay_header_t * hdr, list_t *replay_ll);
int8_t generate_replay(replay_header_t * hdr, list_t * replay_ll, const uint8_t flags);

int8_t save_replay(int fd, replay_header_t * hdr, list_t *replay_ll);
void analyze_replay(uint8_t * buf);
void simulate_replay(sim_replay_t *sim);

#ifdef __cplusplus
}
#endif

#endif

