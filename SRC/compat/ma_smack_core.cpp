/* E1 (MA): Smacker decoder core -- see ma_smack_core.h. */
#include "ma_smack_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}
struct MaSmk {
    AVFormatContext* fmt = 0;
    AVCodecContext*  vctx = 0;
    AVCodecContext*  actx = 0;
    SwrContext*      swr = 0;
    int vidx = -1, aidx = -1;
    AVPacket* pkt = 0;
    AVFrame*  vfr = 0;
    AVFrame*  afr = 0;
    int w = 0, h = 0, pitch = 0;
    double frame_ms = 0.0;
    int arate = 0, achans = 0;
    int findex = -1;
    bool eof = false;
    std::vector<unsigned char> bits;
    unsigned char pal[256*4];
    std::vector<unsigned char> audio;   /* interleaved S16 */
};
static void smk_err(const char* what, int rc) {
    char e[128]; av_strerror(rc, e, sizeof e);
    fprintf(stderr, "[smk] %s: %s\n", what, e);
}
MaSmk* ma_smk_open(const char* path) {
    MaSmk* s = new MaSmk;
    int rc = avformat_open_input(&s->fmt, path, NULL, NULL);
    if (rc < 0) { smk_err("open", rc); delete s; return 0; }
    if ((rc = avformat_find_stream_info(s->fmt, NULL)) < 0) { smk_err("stream info", rc); ma_smk_close(s); return 0; }
    const AVCodec* vc = 0; const AVCodec* ac = 0;
    s->vidx = av_find_best_stream(s->fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &vc, 0);
    if (s->vidx < 0 || !vc) { fprintf(stderr, "[smk] no video stream in %s\n", path); ma_smk_close(s); return 0; }
    s->vctx = avcodec_alloc_context3(vc);
    avcodec_parameters_to_context(s->vctx, s->fmt->streams[s->vidx]->codecpar);
    if ((rc = avcodec_open2(s->vctx, vc, NULL)) < 0) { smk_err("video codec", rc); ma_smk_close(s); return 0; }
    s->w = s->vctx->width; s->h = s->vctx->height;
    s->pitch = ((s->w * 8 + 31) / 32) * 4;
    s->bits.assign((size_t)s->pitch * s->h, 0);
    memset(s->pal, 0, sizeof s->pal);
    AVRational fr = s->fmt->streams[s->vidx]->avg_frame_rate;
    if (fr.num <= 0 || fr.den <= 0) fr = s->fmt->streams[s->vidx]->r_frame_rate;
    s->frame_ms = (fr.num > 0 && fr.den > 0) ? 1000.0 * fr.den / fr.num : 71.0;   /* Smacker default 14 fps */
    s->aidx = av_find_best_stream(s->fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &ac, 0);
    if (s->aidx >= 0 && ac) {
        s->actx = avcodec_alloc_context3(ac);
        avcodec_parameters_to_context(s->actx, s->fmt->streams[s->aidx]->codecpar);
        if (avcodec_open2(s->actx, ac, NULL) < 0) { avcodec_free_context(&s->actx); s->aidx = -1; }
        else {
            s->arate = s->actx->sample_rate;
            s->achans = s->actx->ch_layout.nb_channels;
            AVChannelLayout out; av_channel_layout_default(&out, s->achans);
            if (swr_alloc_set_opts2(&s->swr, &out, AV_SAMPLE_FMT_S16, s->arate,
                                    &s->actx->ch_layout, s->actx->sample_fmt, s->arate, 0, NULL) < 0 ||
                swr_init(s->swr) < 0) { fprintf(stderr, "[smk] resampler failed -> silent\n"); s->arate = 0; }
        }
    }
    s->pkt = av_packet_alloc(); s->vfr = av_frame_alloc(); s->afr = av_frame_alloc();
    return s;
}
void ma_smk_close(MaSmk* s) {
    if (!s) return;
    if (s->pkt) av_packet_free(&s->pkt);
    if (s->vfr) av_frame_free(&s->vfr);
    if (s->afr) av_frame_free(&s->afr);
    if (s->swr) swr_free(&s->swr);
    if (s->vctx) avcodec_free_context(&s->vctx);
    if (s->actx) avcodec_free_context(&s->actx);
    if (s->fmt) avformat_close_input(&s->fmt);
    delete s;
}
int ma_smk_width(const MaSmk* s)  { return s ? s->w : 0; }
int ma_smk_height(const MaSmk* s) { return s ? s->h : 0; }
double ma_smk_frame_ms(const MaSmk* s) { return s ? s->frame_ms : 0.0; }
int ma_smk_audio_rate(const MaSmk* s) { return s ? s->arate : 0; }
int ma_smk_audio_channels(const MaSmk* s) { return s ? s->achans : 0; }
int ma_smk_frame_index(const MaSmk* s) { return s ? s->findex : -1; }
const unsigned char* ma_smk_frame_bits(const MaSmk* s) { return s ? s->bits.data() : 0; }
int ma_smk_pitch(const MaSmk* s) { return s ? s->pitch : 0; }
const unsigned char* ma_smk_palette_bgra(const MaSmk* s) { return s ? s->pal : 0; }
int ma_smk_audio_pending(const MaSmk* s) { return s ? (int)s->audio.size() : 0; }
int ma_smk_take_audio(MaSmk* s, unsigned char* out, int max) {
    if (!s || max <= 0 || s->audio.empty()) return 0;
    int n = (int)s->audio.size(); if (n > max) n = max;
    memcpy(out, s->audio.data(), (size_t)n);
    s->audio.erase(s->audio.begin(), s->audio.begin() + n);
    return n;
}
static void take_video(MaSmk* s) {
    const AVFrame* f = s->vfr;
    if (f->format == AV_PIX_FMT_PAL8) {
        for (int y = 0; y < s->h; ++y)
            memcpy(&s->bits[(size_t)y * s->pitch], f->data[0] + (size_t)y * f->linesize[0], (size_t)s->w);
        if (f->data[1]) memcpy(s->pal, f->data[1], 256 * 4);   /* AV PAL8 entries are BGRA in memory = RGBQUAD */
    } else if (f->format == AV_PIX_FMT_GRAY8) {
        for (int y = 0; y < s->h; ++y)
            memcpy(&s->bits[(size_t)y * s->pitch], f->data[0] + (size_t)y * f->linesize[0], (size_t)s->w);
        for (int i = 0; i < 256; ++i) { s->pal[i*4] = s->pal[i*4+1] = s->pal[i*4+2] = (unsigned char)i; s->pal[i*4+3] = 0; }
    } else {
        static int once = 0;
        if (!once++) fprintf(stderr, "[smk] unexpected pixel format %d\n", (int)f->format);
    }
    s->findex++;
}
static void take_audio(MaSmk* s) {
    if (!s->swr || s->arate <= 0) return;
    const AVFrame* f = s->afr;
    int outn = swr_get_out_samples(s->swr, f->nb_samples);
    if (outn <= 0) return;
    size_t bytes = (size_t)outn * s->achans * 2;
    size_t at = s->audio.size(); s->audio.resize(at + bytes);
    unsigned char* dst = &s->audio[at];
    int got = swr_convert(s->swr, &dst, outn, (const unsigned char* const*)f->extended_data, f->nb_samples);
    if (got < 0) got = 0;
    s->audio.resize(at + (size_t)got * s->achans * 2);
}
int ma_smk_next_frame(MaSmk* s) {
    if (!s) return -1;
    for (;;) {
        /* drain the video decoder first */
        int rc = avcodec_receive_frame(s->vctx, s->vfr);
        if (rc == 0) { take_video(s); av_frame_unref(s->vfr); return 1; }
        if (rc != AVERROR(EAGAIN) && rc != AVERROR_EOF) { smk_err("video decode", rc); return -1; }
        if (s->eof) return 0;
        rc = av_read_frame(s->fmt, s->pkt);
        if (rc < 0) {                       /* end: flush both decoders */
            s->eof = true;
            avcodec_send_packet(s->vctx, NULL);
            if (s->actx) { avcodec_send_packet(s->actx, NULL); while (avcodec_receive_frame(s->actx, s->afr) == 0) { take_audio(s); av_frame_unref(s->afr); } }
            continue;
        }
        if (s->pkt->stream_index == s->vidx) {
            rc = avcodec_send_packet(s->vctx, s->pkt);
            if (rc < 0 && rc != AVERROR(EAGAIN)) smk_err("video packet", rc);
        } else if (s->actx && s->pkt->stream_index == s->aidx) {
            if (avcodec_send_packet(s->actx, s->pkt) == 0)
                while (avcodec_receive_frame(s->actx, s->afr) == 0) { take_audio(s); av_frame_unref(s->afr); }
        }
        av_packet_unref(s->pkt);
    }
}
