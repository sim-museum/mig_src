// JOY-CURVES-1 unit test: the shaping function against hand-computed points.
// g++ -O1 -ISRC/compat tools/joyshape_test.cpp -o /tmp/joyshape_test && /tmp/joyshape_test
#include "ma_joyshape.h"
#include <cstdio>
#include <cstdlib>

static int fails = 0;
static void eq(const char* what, int got, int want, int tol = 1) {
    if (abs(got - want) > tol) { printf("FAIL %s: got %d want %d\n", what, got, want); fails++; }
}
static MaAxisShape S(const char* t) { MaAxisShape s; char e[128]; if (!ma_shape_parse(&s, t, e, sizeof e)) { printf("parse \"%s\": %s\n", t, e); exit(1); } return s; }

int main() {
    MaAxisShape off; ma_shape_default(&off);                       // no shape line: bit for bit
    for (int r = -32768; r <= 32767; r += 7) if (ma_shape_apply(&off, r) != r) { printf("FAIL default changes %d\n", r); fails++; break; }
    MaAxisShape lin = S("curve linear");                           // linear, shaped: within 1 of the input
    eq("linear 16384", ma_shape_apply(&lin, 16384), 16384); eq("linear -32768", ma_shape_apply(&lin, -32768), -32768);
    eq("linear 32767", ma_shape_apply(&lin, 32767), 32767);
    MaAxisShape sq = S("curve square");                            // half travel -> 25 %
    eq("square 0.5", ma_shape_apply(&sq, 16384), (int)(0.25 * 32767), 40);
    eq("square -0.5", ma_shape_apply(&sq, -16384), -(int)(0.25 * 32768), 40);
    eq("square full", ma_shape_apply(&sq, 32767), 32767);
    MaAxisShape db = S("deadband 10");                             // inside the band 0; edge of band 0; 55 % -> 50 %
    eq("db 3000", ma_shape_apply(&db, 3000), 0); eq("db 0.1", ma_shape_apply(&db, 3277), 0, 2);
    eq("db 0.55", ma_shape_apply(&db, (int)(0.55 * 32767)), (int)(0.5 * 32767), 40);
    eq("db full", ma_shape_apply(&db, 32767), 32767);
    MaAxisShape inv = S("invert");
    eq("invert 10000", ma_shape_apply(&inv, 10000), -10000, 2); eq("invert min", ma_shape_apply(&inv, -32768), 32767);
    MaAxisShape one = S("mode onesided deadband 5");               // throttle: bottom 5 % reads minimum
    eq("onesided low", ma_shape_apply(&one, -31000), -32768); eq("onesided top", ma_shape_apply(&one, 32767), 32767);
    eq("onesided mid", ma_shape_apply(&one, 0), (int)((0.5 - 0.05) / 0.95 * 65535 - 32768), 40);
    MaAxisShape pts = S("curve 5 10 20 30 40 50 60 70 85 100");    // hand points: 15 % -> 7.5 %
    eq("points 0.15", ma_shape_apply(&pts, (int)(0.15 * 32767)), (int)(0.075 * 32767), 40);
    MaAxisShape f = S("filter 50");                                // tau 100 ms: a +-2 % jitter at 60 Hz shrinks
    float st = 0; int pr = 0; double sum2 = 0; int n = 0; srand(1);
    for (int i = 0; i < 600; i++) { int v = 10000 + (rand() % 1311) - 655; int o = ma_shape_filter(&f, &st, &pr, v, 16.7f);
        if (i > 60) { sum2 += (o - 10000.0) * (o - 10000.0); n++; } }
    const double rms = sqrt(sum2 / n);
    printf("filter 50: rms jitter %.0f of raw ~378\n", rms);
    if (rms > 150) { printf("FAIL filter too weak\n"); fails++; }
    char e[128]; MaAxisShape bad;
    if (ma_shape_parse(&bad, "curve 10 20", e, sizeof e) || ma_shape_parse(&bad, "deadband 99", e, sizeof e)
        || ma_shape_parse(&bad, "wobble 3", e, sizeof e) || ma_shape_parse(&bad, "curve 50 40 50 60 70 80 90 95 99 100", e, sizeof e)) { printf("FAIL bad lines accepted\n"); fails++; }
    printf(fails ? "JOYSHAPE: %d FAIL\n" : "JOYSHAPE: PASS\n", fails);
    return fails != 0;
}
