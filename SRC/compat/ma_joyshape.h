/* JOY-CURVES-1 (controllers track C8): per-axis shaping for the port's joystick bridge (bob_video.cpp).
   Pure functions, so tools/joyshape_test.cpp can check them against hand-computed points.

   joystick.txt line:   shape <slot> = [deadband <pct>] [curve linear|square|expo|<10 numbers>] [filter <0-100>]
                                       [invert] [mode centred|onesided]
   - deadband: percent of travel from the centre (centred) or from the low end (onesided) that reads 0 / minimum.
   - curve: output in percent at 10%, 20% ... 100% of travel; linear interpolation between, (0,0) implied.
     linear = 10 20 ... 100 (default), square = 1 4 9 ... 100 (output = travel^2), expo = 0.5 x + 0.5 x^3.
   - filter: smoothing; 0 off, 100 heaviest. Time-based (tau = filter^2 / 25 ms: 50 -> 100 ms, 100 -> 400 ms), so the
     result does not depend on how often the game polls.
   - invert: flips the axis. mode onesided: for throttles and brakes (shaped from the low end, not from the centre).
   Every step maps the physical extremes to themselves, so Rowan's calibration still sees full travel. A slot
   without a shape line passes through untouched (bit for bit). */
#pragma once
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct MaAxisShape {
    int   on;            /* 0: pass through */
    float deadband;      /* 0..0.95, fraction */
    float curve[11];     /* output at 0, 0.1 .. 1.0 */
    float filter;        /* 0..100 */
    int   invert, onesided;
};

static inline void ma_shape_preset(MaAxisShape* s, const char* name) {
    for (int i = 0; i <= 10; i++) {
        const float x = i / 10.0f;
        s->curve[i] = !strcmp(name, "square") ? x * x : !strcmp(name, "expo") ? 0.5f * x + 0.5f * x * x * x : x;
    }
}
static inline void ma_shape_default(MaAxisShape* s) { memset(s, 0, sizeof *s); ma_shape_preset(s, "linear"); }

/* parse the right-hand side of a shape line; returns 0 and an error text on failure */
static inline int ma_shape_parse(MaAxisShape* s, const char* text, char* err, size_t en) {
    ma_shape_default(s); s->on = 1;
    char buf[256]; snprintf(buf, sizeof buf, "%s", text);
    char* save = 0;
    for (char* w = strtok_r(buf, " \t\r\n", &save); w; w = strtok_r(0, " \t\r\n", &save)) {
        if (!strcmp(w, "deadband")) { char* v = strtok_r(0, " \t\r\n", &save); if (!v) { snprintf(err, en, "deadband needs a number"); return 0; }
            float d = (float)atof(v) / 100.0f; if (d < 0 || d > 0.95f) { snprintf(err, en, "deadband %s out of 0..95", v); return 0; } s->deadband = d; }
        else if (!strcmp(w, "curve")) { char* v = strtok_r(0, " \t\r\n", &save); if (!v) { snprintf(err, en, "curve needs a name or 10 numbers"); return 0; }
            if (!strcmp(v, "linear") || !strcmp(v, "square") || !strcmp(v, "expo")) ma_shape_preset(s, v);
            else { s->curve[0] = 0; s->curve[1] = (float)atof(v) / 100.0f;
                for (int i = 2; i <= 10; i++) { char* x = strtok_r(0, " \t\r\n", &save); if (!x) { snprintf(err, en, "curve needs 10 numbers"); return 0; } s->curve[i] = (float)atof(x) / 100.0f; }
                for (int i = 1; i <= 10; i++) if (s->curve[i] < s->curve[i - 1]) { snprintf(err, en, "curve must not fall"); return 0; } } }
        else if (!strcmp(w, "filter")) { char* v = strtok_r(0, " \t\r\n", &save); if (!v) { snprintf(err, en, "filter needs 0..100"); return 0; }
            float f = (float)atof(v); if (f < 0 || f > 100) { snprintf(err, en, "filter %s out of 0..100", v); return 0; } s->filter = f; }
        else if (!strcmp(w, "invert")) s->invert = 1;
        else if (!strcmp(w, "mode")) { char* v = strtok_r(0, " \t\r\n", &save);
            if (v && !strcmp(v, "onesided")) s->onesided = 1; else if (v && (!strcmp(v, "centred") || !strcmp(v, "centered"))) s->onesided = 0;
            else { snprintf(err, en, "mode is centred or onesided"); return 0; } }
        else { snprintf(err, en, "unknown word \"%s\"", w); return 0; }
    }
    return 1;
}

static inline float ma_shape_curve(const MaAxisShape* s, float a) {   /* a in 0..1 */
    if (a <= 0) return 0; if (a >= 1) return s->curve[10];
    const float p = a * 10.0f; const int i = (int)p; const float f = p - i;
    return s->curve[i] + f * (s->curve[i + 1] - s->curve[i]);
}
/* raw -32768..32767 -> shaped -32768..32767 (no filter; see ma_shape_filter) */
static inline int ma_shape_apply(const MaAxisShape* s, int raw) {
    if (!s->on) return raw;
    float y;
    if (s->onesided) {
        float u = (raw + 32768) / 65535.0f;                                  /* 0..1 from the low end */
        u = u <= s->deadband ? 0 : (u - s->deadband) / (1 - s->deadband);
        y = 2.0f * ma_shape_curve(s, u) - 1.0f;
    } else {
        float x = raw >= 0 ? raw / 32767.0f : raw / 32768.0f; const float sg = x < 0 ? -1.0f : 1.0f; float a = fabsf(x);
        a = a <= s->deadband ? 0 : (a - s->deadband) / (1 - s->deadband);
        y = sg * ma_shape_curve(s, a);
    }
    if (s->invert) y = -y;
    int r = (int)lrintf(y >= 0 ? y * 32767.0f : y * 32768.0f);
    return r < -32768 ? -32768 : r > 32767 ? 32767 : r;
}
/* low-pass with time constant tau = filter^2/25 ms; state is per axis; dt_ms since the last call */
static inline int ma_shape_filter(const MaAxisShape* s, float* state, int* primed, int v, float dt_ms) {
    if (!s->on || s->filter <= 0) return v;
    if (!*primed) { *state = (float)v; *primed = 1; return v; }
    const float tau = s->filter * s->filter / 25.0f;
    const float k = dt_ms <= 0 ? 0 : 1.0f - expf(-dt_ms / tau);
    *state += k * ((float)v - *state);
    return (int)lrintf(*state);
}
