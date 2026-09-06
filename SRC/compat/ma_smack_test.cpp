/* smacktest <file.smk>: decode everything headlessly and print what the gate compares to ffprobe. */
#include "ma_smack_core.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: smacktest file.smk\n"); return 2; }
    MaSmk* s = ma_smk_open(argv[1]);
    if (!s) return 1;
    long frames = 0, nonblack = 0; long audio_bytes = 0; unsigned char buf[65536];
    int rc;
    while ((rc = ma_smk_next_frame(s)) == 1) {
        frames++;
        const unsigned char* b = ma_smk_frame_bits(s); const unsigned char* p = ma_smk_palette_bgra(s);
        int w = ma_smk_width(s), h = ma_smk_height(s), pitch = ma_smk_pitch(s);
        long lit = 0;
        for (int y = 0; y < h; y += 8) for (int x = 0; x < w; x += 8) { const unsigned char* c = p + 4 * b[y*pitch + x]; if (c[0] + c[1] + c[2] > 48) lit++; }
        if (lit > 0) nonblack++;
        int n; while ((n = ma_smk_take_audio(s, buf, sizeof buf)) > 0) audio_bytes += n;
    }
    int n; while ((n = ma_smk_take_audio(s, buf, sizeof buf)) > 0) audio_bytes += n;
    printf("file=%s width=%d height=%d frame_ms=%.2f frames=%ld nonblack=%ld audio_rate=%d channels=%d audio_samples=%ld rc=%d\n",
           argv[1], ma_smk_width(s), ma_smk_height(s), ma_smk_frame_ms(s), frames, nonblack,
           ma_smk_audio_rate(s), ma_smk_audio_channels(s),
           ma_smk_audio_channels(s) > 0 ? audio_bytes / (2 * ma_smk_audio_channels(s)) : 0L, rc);
    ma_smk_close(s);
    return rc < 0 ? 1 : 0;
}
