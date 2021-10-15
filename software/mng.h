
//  author:          Petre Rodan <2b4eda@subdimension.ro>
//  available from:  https://github.com/rodan/sigdup
//  license:         GNU GPLv3

#ifndef __MNG_H__
#define __MNG_H__

#ifdef __cplusplus
extern "C" {
#endif

int8_t parse_signal(input_sig_t * s, replay_header_t * hdr, list_t *replay_ll);
int8_t save_replay(int fd, replay_header_t * hdr, list_t *replay_ll);
void analyze_replay(uint8_t * buf);

#ifdef __cplusplus
}
#endif

#endif

