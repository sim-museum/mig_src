/* E1 (MA): Smacker (.smk) decoding for the port, on libavformat/libavcodec (the box's ffmpeg 8).
   No game types here so the same core can be built into a headless CLI (smacktest) that the gate runs. */
#ifndef MA_SMACK_CORE_H
#define MA_SMACK_CORE_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MaSmk MaSmk;
/* open; returns NULL (and prints why) on failure */
MaSmk* ma_smk_open(const char* path);
void   ma_smk_close(MaSmk* s);
int    ma_smk_width(const MaSmk* s);
int    ma_smk_height(const MaSmk* s);
double ma_smk_frame_ms(const MaSmk* s);          /* nominal ms per video frame */
int    ma_smk_audio_rate(const MaSmk* s);        /* 0 = no audio track */
int    ma_smk_audio_channels(const MaSmk* s);
/* Decode until the NEXT video frame is ready (audio met on the way is appended to the audio queue).
   Returns 1 when a frame is ready, 0 at end of stream, -1 on error. */
int    ma_smk_next_frame(MaSmk* s);
int    ma_smk_frame_index(const MaSmk* s);       /* 0-based index of the frame last returned */
/* the current frame as 8-bit indices, top-down, pitch = ma_smk_pitch(), plus 256 BGRA palette entries */
const unsigned char* ma_smk_frame_bits(const MaSmk* s);
int    ma_smk_pitch(const MaSmk* s);
const unsigned char* ma_smk_palette_bgra(const MaSmk* s);   /* 256 x 4 bytes (RGBQUAD order) */
/* drain up to `max` bytes of interleaved S16 PCM decoded so far; returns bytes copied */
int    ma_smk_take_audio(MaSmk* s, unsigned char* out, int max);
int    ma_smk_audio_pending(const MaSmk* s);
#ifdef __cplusplus
}
#endif
#endif
