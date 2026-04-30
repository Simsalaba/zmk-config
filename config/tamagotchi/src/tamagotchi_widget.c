/*
 * EMO-style robot eyes widget for ZMK nice!view peripheral display.
 *
 * Expressive rounded-rectangle eyes on 160×68 monochrome Sharp Memory LCD.
 * 60 fps animation with ease-out interpolation, natural blinks, gaze shifts,
 * sub-expressions, and personality-driven state machine.
 *
 * States:
 *   HAPPY     – smiling eyes with upward curve, gentle bounce (typing)
 *   NEUTRAL   – default pill-block eyes, wandering gaze, blinks,
 *               random curious / thinking / wink sub-expressions
 *   SLEEPY    – half-closed, slow heavy movements
 *   ASLEEP    – thin closed lines, breathing motion, floating zzz
 *   ANNOYED   – narrow flat eyes, grumpy glare (random wake during sleep)
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#include "tamagotchi_widget.h"

/* ═══════════════════════════════════════════════════════════
 * Configuration
 * ═══════════════════════════════════════════════════════════ */

#define DISP_W    160
#define DISP_H    68
#define TICK_MS   16           /* ~60 fps */

/* Fixed-point scale (×16) for sub-pixel easing */
#define FP  16
#define fp(v)   ((v) * FP)
#define unfp(v) ((v) / FP)

/* ── Eye resting positions ────────────────────── */
#define EYE_L_CX   50
#define EYE_R_CX  110
#define EYE_CY     34

/* ── Eye dimensions per expression (pixels) ───── */
#define NEUT_W    30          /* neutral: wider than tall, EMO-style */
#define NEUT_H    24
#define NEUT_CR   10

#define HAPPY_W   32          /* happy: looking down at hands, sparkly */
#define HAPPY_H   26
#define HAPPY_CR  10

#define SURP_W    34          /* (unused, kept for sizing reference) */
#define SURP_H    30
#define SURP_CR   12

#define SLEEPY_W  28          /* drowsy half-closed */
#define SLEEPY_H  12
#define SLEEPY_CR  5

#define ASLEEP_W  28          /* fully asleep */
#define ASLEEP_H   2
#define ASLEEP_CR  1

#define ANNOY_W   32          /* annoyed / grumpy: wide and flat */
#define ANNOY_H    8
#define ANNOY_CR   3

/* ── State timeouts (ms) ──────────────────────── */
#define IDLE_MS       10000   /* 10 s  → NEUTRAL  */
#define SLEEPY_MS    120000   /* 2 min → SLEEPY   */
#define ASLEEP_MS    240000   /* 4 min → ASLEEP   */
#define GRUMPY_MS    600000   /* 10 min → maybe grumpy */

/* ── Easing speeds (1..FP, higher = snappier) ─── */
#define EASE_FAST      8     /* ~60 ms  – blinks, snaps    */
#define EASE_NORMAL    5     /* ~130 ms – gaze, expressions */
#define EASE_SLOW      2     /* ~280 ms – sleepy moves      */
#define EASE_GLACIAL   1     /* ~500 ms – breathing          */

/* ── Burst detection ──────────────────────────── */
#define BURST_WINDOW_MS  1500
#define BURST_COUNT      5

/* ═══════════════════════════════════════════════════════════
 * Internal types
 * ═══════════════════════════════════════════════════════════ */

enum emo_state {
    ST_HAPPY,
    ST_NEUTRAL,
    ST_SLEEPY,
    ST_ASLEEP,
    ST_ANNOYED,
};

enum idle_sub {
    SUB_NONE,
    SUB_LOOK_LEFT,
    SUB_LOOK_RIGHT,
    SUB_LOOK_UP,
    SUB_CURIOUS,
    SUB_THINKING,
    SUB_WINK,
};

/* All values in FP units */
struct eye_fp {
    int lw, lh, lcr;          /* left  eye w/h/corner-radius */
    int rw, rh, rcr;          /* right eye                   */
    int dx, dy;                /* shared gaze offset          */
};

/* ═══════════════════════════════════════════════════════════
 * Static state
 * ═══════════════════════════════════════════════════════════ */

/* Animation */
static struct eye_fp cur, tgt;
static int ease_spd;               /* general easing   */
static int ease_h_spd;             /* height (separate for blinks) */

/* State machine */
static enum emo_state state;
static int64_t last_activity;
static uint32_t frame;

/* Sub-expressions (neutral idle) */
static enum idle_sub sub_expr;
static int sub_timer;
static int sub_cooldown;

/* Auto-return timer (surprised / annoyed → target state) */
static int return_timer;
static enum emo_state return_state;

/* Grumpy wake-up */
static int grumpy_cooldown;

/* Blink system */
enum bl_phase { BL_IDLE, BL_CLOSING, BL_HOLD, BL_OPENING };
static enum bl_phase bl_phase;
static int bl_timer;
static int bl_cooldown;
static bool bl_wink;               /* true = only left eye blinks */

/* Burst detection */
static int64_t burst_ring[BURST_COUNT];
static int burst_idx;

/* Typing heat (controls HAPPY entry — needs sustained typing) */
static int key_heat;               /* +4 per keypress, -1 per frame */
#define HAPPY_HEAT_THRESHOLD 8     /* ~3 quick keys to enter HAPPY */

/* Sparkle ramp (delays sparkle/marks appearance) */
static int sparkle_ramp;           /* frames spent in HAPPY */
#define SPARKLE_DELAY 30           /* ~0.5 s before sparkles show */

/* Canvas */
static lv_color_t cbuf[DISP_W * DISP_H];
static lv_obj_t  *canvas_obj;
static lv_color_t bg, fg;

static struct k_work_delayable anim_work;

/* ═══════════════════════════════════════════════════════════
 * PRNG  (simple LCG, seeded from uptime)
 * ═══════════════════════════════════════════════════════════ */

static uint32_t rng_s;

static uint32_t rng(void)
{
    rng_s = rng_s * 1103515245u + 12345u;
    return (rng_s >> 16) & 0x7fff;
}

static int rng_range(int lo, int hi)
{
    if (lo >= hi) return lo;
    return lo + (int)(rng() % (unsigned)(hi - lo + 1));
}

/* ═══════════════════════════════════════════════════════════
 * Helpers
 * ═══════════════════════════════════════════════════════════ */

static inline void set_px(int x, int y, lv_color_t c)
{
    if ((unsigned)x < DISP_W && (unsigned)y < DISP_H)
        lv_canvas_set_px_color(canvas_obj, x, y, c);
}

static void ease_toward(int *c, int t, int spd)
{
    int d = t - *c;
    if (d == 0) return;
    int delta = d * spd / FP;
    if (delta == 0) delta = (d > 0) ? 1 : -1;
    *c += delta;
}

/* ═══════════════════════════════════════════════════════════
 * Drawing – eye with iris, highlight, thick lid, and glow
 *
 * Structure (inside→out):
 *   pupil     → small black circle (inverted glint)
 *   iris      → white rounded-rect filling most of interior
 *   sclera    → white border (thick on top = eyelid)
 *   glow      → dithered fade (50 % → 25 %)
 * ═══════════════════════════════════════════════════════════ */

#define GLOW_INNER 2
#define GLOW_OUTER 1
#define GLOW_TOTAL (GLOW_INNER + GLOW_OUTER)

#define LID_TOP    4          /* thick upper eyelid */
#define LID_SIDE   3          /* sclera border sides */
#define LID_BOT    3          /* sclera border bottom */
#define HL_R       3          /* highlight circle radius */

static bool pt_in_rrect(int px, int py, int w, int h, int cr)
{
    if (px < 0 || py < 0 || px >= w || py >= h) return false;
    if (cr <= 0) return true;
    int dx = 0, dy = 0;
    if      (px < cr)         dx = cr - px;
    else if (px > w - 1 - cr) dx = px - (w - 1 - cr);
    if      (py < cr)         dy = cr - py;
    else if (py > h - 1 - cr) dy = py - (h - 1 - cr);
    return !(dx > 0 && dy > 0 && dx * dx + dy * dy > cr * cr);
}

static bool pt_in_circle(int px, int py, int cx, int cy, int r)
{
    int dx = px - cx, dy = py - cy;
    return dx * dx + dy * dy <= r * r;
}

static void draw_eye(int cx, int cy, int w, int h, int cr,
                     lv_color_t c, int gx, int gy)
{
    if (w <= 0 || h <= 0) return;

    /* total rect including glow */
    int tw  = w  + GLOW_TOTAL * 2;
    int th  = h  + GLOW_TOTAL * 2;
    int tcr = cr + GLOW_TOTAL;
    if (tcr > tw / 2) tcr = tw / 2;
    if (tcr > th / 2) tcr = th / 2;

    /* middle rect (inner glow boundary) */
    int mw  = w  + GLOW_INNER * 2;
    int mh  = h  + GLOW_INNER * 2;
    int mcr = cr + GLOW_INNER;
    if (mcr > mw / 2) mcr = mw / 2;
    if (mcr > mh / 2) mcr = mh / 2;

    int x0 = cx - tw / 2;
    int y0 = cy - th / 2;

    /* iris (dark interior) — inset from core, shifted down for thick lid */
    bool use_iris = (w >= 12 && h >= 12);
    int iw  = w - LID_SIDE * 2;
    int ih  = h - LID_TOP - LID_BOT;
    int icr = cr > 4 ? cr - 3 : (cr > 1 ? cr - 1 : 0);
    if (icr > iw / 2) icr = iw / 2;
    if (icr > ih / 2) icr = ih / 2;
    /* iris origin relative to core origin */
    int ix0 = LID_SIDE;
    int iy0 = LID_TOP;

    /* highlight position: centered in iris, offset by gaze */
    int hlx = ix0 + iw / 2 + gx / 3;
    int hly = iy0 + ih / 2 + gy / 3;

    for (int py = 0; py < th; py++) {
        for (int px = 0; px < tw; px++) {
            if (!pt_in_rrect(px, py, tw, th, tcr)) continue;

            int sx = x0 + px;
            int sy = y0 + py;
            int cpx = px - GLOW_TOTAL;     /* core-relative */
            int cpy = py - GLOW_TOTAL;
            int mpx = px - GLOW_OUTER;
            int mpy = py - GLOW_OUTER;

            if (pt_in_rrect(cpx, cpy, w, h, cr)) {
                /* inside core shape */
                if (use_iris &&
                    pt_in_rrect(cpx - ix0, cpy - iy0, iw, ih, icr)) {
                    /* iris zone — check highlight first */
                    if (pt_in_circle(cpx, cpy, hlx, hly, HL_R)) {
                        set_px(sx, sy, bg);             /* pupil (black) */
                    } else {
                        set_px(sx, sy, c);              /* iris (white) */
                    }
                } else {
                    set_px(sx, sy, c);                  /* sclera (white) */
                }
            } else if (pt_in_rrect(mpx, mpy, mw, mh, mcr)) {
                if ((sx + sy) % 2 == 0) set_px(sx, sy, c); /* 50 % glow */
            } else {
                if (sx % 2 == 0 && sy % 2 == 0)
                    set_px(sx, sy, c);                  /* 25 % glow */
            }
        }
    }

    /* ── Cat-eye wing / integrated lash ──────────────────────
     * The thick upper lid tapers outward+upward at both outer
     * corners, creating a winged eyeliner look.  Drawn as
     * solid sclera pixels extending from the corner.
     * Only when eye is tall enough to have a visible lid. */
    if (h >= 6) {
        int ey_top = cy - h / 2;           /* top edge of core */
        for (int side = 0; side < 2; side++) {
            int dir = (side == 0) ? -1 : 1; /* left wing, right wing */
            int wx  = cx + dir * (w / 2);    /* outer edge x */

            /* wing: 5 pixels sweeping outward-upward, 2px thick at base */
            set_px(wx + dir * 1, ey_top,     c);
            set_px(wx + dir * 1, ey_top - 1, c);
            set_px(wx + dir * 2, ey_top - 1, c);
            set_px(wx + dir * 2, ey_top - 2, c);
            set_px(wx + dir * 3, ey_top - 2, c);
            set_px(wx + dir * 3, ey_top - 3, c);
            set_px(wx + dir * 4, ey_top - 3, c); /* taper to 1px */
            set_px(wx + dir * 4, ey_top - 4, c);
            set_px(wx + dir * 5, ey_top - 5, c); /* tip */
        }
    }
}

/* plain solid rrect (used for non-eye shapes like zzz) */
static void draw_rrect(int cx, int cy, int w, int h, int cr,
                       lv_color_t c)
{
    if (w <= 0 || h <= 0) return;
    int x0 = cx - w / 2;
    int y0 = cy - h / 2;
    if (cr > w / 2) cr = w / 2;
    if (cr > h / 2) cr = h / 2;
    if (cr < 0) cr = 0;

    for (int py = 0; py < h; py++)
        for (int px = 0; px < w; px++)
            if (pt_in_rrect(px, py, w, h, cr))
                set_px(x0 + px, y0 + py, c);
}

/* ═══════════════════════════════════════════════════════════
 * Drawing – 'z' character (sz × sz pixels) + floating zzz
 * ═══════════════════════════════════════════════════════════ */

static void draw_z(int x0, int y0, int sz, lv_color_t c)
{
    if (sz < 2) return;
    /* top and bottom bars (2 px thick for visibility) */
    for (int i = 0; i < sz; i++) {
        set_px(x0 + i, y0, c);
        set_px(x0 + i, y0 + 1, c);
        set_px(x0 + i, y0 + sz - 1, c);
        set_px(x0 + i, y0 + sz - 2, c);
    }
    /* diagonal (2 px thick) */
    for (int i = 1; i < sz - 1; i++) {
        set_px(x0 + sz - 1 - i, y0 + i, c);
        set_px(x0 + sz - i,     y0 + i, c);
    }
}

static void draw_zzz(void)
{
    int cycle = frame % 120;                /* 2 s loop */

    for (int i = 0; i < 3; i++) {
        int phase = (cycle + i * 40) % 120; /* staggered */
        if (phase >= 80) continue;          /* hidden part of cycle */

        int sz = 5 + i * 2;                /* larger: 5, 7, 9 px */
        int x  = EYE_R_CX + 16 + i * 10 + phase / 10;
        int y  = EYE_CY   - 12 - phase / 3;

        if (y >= 0 && y + sz < DISP_H && x + sz < DISP_W)
            draw_z(x, y, sz, fg);
    }
}

/* ═══════════════════════════════════════════════════════════
 * Drawing – anime sparkle (happy eyes only)
 *
 * Adds a twinkle star near the existing highlight and
 * small radiating lines for extra sparkle.
 * ═══════════════════════════════════════════════════════════ */

static void draw_sparkle(int eye_cx, int eye_cy, int ew, int eh)
{
    if (eh < 12 || ew < 12) return;

    /* position: near the highlight (upper-left quadrant of iris) */
    int sx = eye_cx - ew / 6;
    int sy = eye_cy - eh / 6;

    /* star/cross: + shape */
    set_px(sx, sy, fg);
    set_px(sx - 1, sy, fg);
    set_px(sx + 1, sy, fg);
    set_px(sx, sy - 1, fg);
    set_px(sx, sy + 1, fg);

    /* diagonal rays */
    set_px(sx - 1, sy - 1, fg);
    set_px(sx + 1, sy - 1, fg);
    set_px(sx - 1, sy + 1, fg);
    set_px(sx + 1, sy + 1, fg);

    /* extended rays for extra twinkle */
    set_px(sx, sy - 2, fg);
    set_px(sx, sy + 2, fg);
    set_px(sx - 2, sy, fg);
    set_px(sx + 2, sy, fg);
}

/* ═══════════════════════════════════════════════════════════
 * Drawing – kawaii happy marks (joy lines below each eye)
 * ═══════════════════════════════════════════════════════════ */

static void draw_happy_marks(int cx, int cy, int eh)
{
    int yb = cy + eh / 2 + 2;              /* just below the eye */

    /* two small diagonal lines: ╲ ╲  (manga blush/joy marks) */
    for (int m = 0; m < 2; m++) {
        int mx = cx - 3 + m * 5;
        for (int i = 0; i < 3; i++) {
            set_px(mx + i,     yb + i, fg);
            set_px(mx + i + 1, yb + i, fg); /* 2 px thick */
        }
    }
}

/* ═══════════════════════════════════════════════════════════
 * Drawing – nose dot (tiny mark between the eyes)
 *
 * Triggers face pareidolia even without a head frame.
 * Moves slightly with vertical gaze.
 * ═══════════════════════════════════════════════════════════ */

static void draw_nose(int gaze_y)
{
    int nx = (EYE_L_CX + EYE_R_CX) / 2;   /* centered between eyes */
    int ny = EYE_CY + 8 + gaze_y / 3;      /* below eye line, subtle gaze follow */

    /* small 3-pixel inverted-V shape: · ·  */
    /*                                  ·   */
    set_px(nx - 1, ny,     fg);
    set_px(nx + 1, ny,     fg);
    set_px(nx,     ny + 1, fg);
}

static void schedule_blink(void)
{
    bl_cooldown = rng_range(120, 240);     /* 2-4 s at 60 fps */
    bl_phase    = BL_IDLE;
    bl_wink     = false;
}

static bool is_blinking(void)
{
    return bl_phase == BL_CLOSING || bl_phase == BL_HOLD;
}

static void blink_tick(void)
{
    /* Suppress blinks only when eyes are already closed */
    if (state == ST_ASLEEP)
        return;

    switch (bl_phase) {

    case BL_IDLE:
        if (--bl_cooldown <= 0) {
            bl_phase = BL_CLOSING;
            bl_timer = 0;
            bl_wink  = (state == ST_NEUTRAL && rng() % 15 == 0);
        }
        break;

    case BL_CLOSING: {
        bl_timer++;
        int dur = (state == ST_SLEEPY) ? 8 : 3;
        if (bl_timer >= dur) {
            bl_phase = BL_HOLD;
            bl_timer = (state == ST_SLEEPY) ? 3 : 1;
        }
        break;
    }

    case BL_HOLD:
        if (--bl_timer <= 0) {
            bl_phase = BL_OPENING;
            bl_timer = 0;
        }
        break;

    case BL_OPENING: {
        bl_timer++;
        int dur = (state == ST_SLEEPY) ? 10 : 4;
        if (bl_timer >= dur) {
            schedule_blink();
        }
        break;
    }
    }
}

/* ═══════════════════════════════════════════════════════════
 * Burst detection
 * ═══════════════════════════════════════════════════════════ */

static bool detect_burst(void)
{
    int64_t now = k_uptime_get();
    burst_ring[burst_idx] = now;
    burst_idx = (burst_idx + 1) % BURST_COUNT;

    int64_t oldest = burst_ring[burst_idx];
    return (oldest > 0 && (now - oldest) < BURST_WINDOW_MS);
}

/* ═══════════════════════════════════════════════════════════
 * Sub-expressions (idle personality)
 * ═══════════════════════════════════════════════════════════ */

static void pick_sub(void)
{
    switch (rng_range(0, 5)) {
    case 0: sub_expr = SUB_LOOK_LEFT;  sub_timer = rng_range(60, 120);  break;
    case 1: sub_expr = SUB_LOOK_RIGHT; sub_timer = rng_range(60, 120);  break;
    case 2: sub_expr = SUB_LOOK_UP;    sub_timer = rng_range(45, 90);   break;
    case 3: sub_expr = SUB_CURIOUS;    sub_timer = rng_range(60, 100);  break;
    case 4: sub_expr = SUB_THINKING;   sub_timer = rng_range(90, 150);  break;
    case 5: sub_expr = SUB_WINK;       sub_timer = rng_range(10, 20);   break;
    }
}

static void update_sub(void)
{
    if (state != ST_NEUTRAL) {
        sub_expr     = SUB_NONE;
        sub_cooldown = 60;
        return;
    }
    if (sub_expr != SUB_NONE) {
        if (--sub_timer <= 0) {
            sub_expr     = SUB_NONE;
            sub_cooldown = rng_range(300, 900);  /* 5-15 s */
        }
    } else {
        if (--sub_cooldown <= 0)
            pick_sub();
    }
}

/* ═══════════════════════════════════════════════════════════
 * State machine
 * ═══════════════════════════════════════════════════════════ */

static void set_state(enum emo_state s)
{
    state        = s;
    sub_expr     = SUB_NONE;
    sub_cooldown = rng_range(300, 600);
}

static void update_state(void)
{
    /* Auto-return from ANNOYED */
    if (return_timer > 0 && --return_timer == 0) {
        set_state(return_state);
        if (return_state == ST_ASLEEP)
            grumpy_cooldown = rng_range(3600, 10800);
        return;
    }

    int64_t elapsed = k_uptime_get() - last_activity;

    switch (state) {
    case ST_HAPPY:
        if (elapsed >= IDLE_MS) set_state(ST_NEUTRAL);
        break;

    case ST_NEUTRAL:
        if (elapsed >= SLEEPY_MS) set_state(ST_SLEEPY);
        break;

    case ST_SLEEPY:
        if (elapsed >= ASLEEP_MS) {
            set_state(ST_ASLEEP);
            grumpy_cooldown = rng_range(3600, 10800); /* 1-3 min */
        }
        break;

    case ST_ASLEEP:
        if (elapsed >= GRUMPY_MS && --grumpy_cooldown <= 0) {
            if (rng() % 3 == 0) {
                set_state(ST_ANNOYED);
                return_timer = rng_range(120, 300);   /* 2-5 s */
                return_state = ST_ASLEEP;
            } else {
                grumpy_cooldown = rng_range(3600, 7200);
            }
        }
        break;

    default:
        break;
    }
}

/* ═══════════════════════════════════════════════════════════
 * Expression targets
 * ═══════════════════════════════════════════════════════════ */

static void compute_targets(void)
{
    ease_spd   = EASE_NORMAL;
    ease_h_spd = EASE_NORMAL;

    switch (state) {

    /* ── HAPPY ── smiling eyes (upward-curving squint) ── */
    case ST_HAPPY: {
        /* Smiling eyes: wide but vertically squished = happy squint */
        tgt.lw = tgt.rw = fp(HAPPY_W);
        tgt.lh = tgt.rh = fp(14);            /* squinted closed */
        tgt.lcr = tgt.rcr = fp(HAPPY_CR);

        /* gentle downward gaze with sway */
        tgt.dy = fp(4);
        int hp = frame % 60;
        if      (hp < 20) tgt.dx = fp(-2);
        else if (hp < 40) tgt.dx = fp(2);
        else              tgt.dx = fp(0);

        ease_h_spd = EASE_FAST;
        break;
    }

    /* ── NEUTRAL ── default with dramatic looking around ── */
    case ST_NEUTRAL: {
        tgt.lw  = tgt.rw  = fp(NEUT_W);
        tgt.lh  = tgt.rh  = fp(NEUT_H);
        tgt.lcr = tgt.rcr = fp(NEUT_CR);

        /* micro-saccade (3 s cycle) */
        int mc = frame % 180;
        if      (mc < 60)  { tgt.dx = 0;       tgt.dy = 0;       }
        else if (mc < 90)  { tgt.dx = fp(-6);  tgt.dy = fp(2);   }
        else if (mc < 120) { tgt.dx = fp(6);   tgt.dy = fp(-2);  }
        else               { tgt.dx = 0;       tgt.dy = fp(-4);  }

        /* sub-expression overrides — big sweeping gaze */
        switch (sub_expr) {
        case SUB_LOOK_LEFT:
            tgt.dx = fp(-30);  tgt.dy = fp(0);
            break;
        case SUB_LOOK_RIGHT:
            tgt.dx = fp(30);   tgt.dy = fp(0);
            break;
        case SUB_LOOK_UP:
            tgt.dx = fp(0);    tgt.dy = fp(-12);
            break;
        case SUB_CURIOUS:
            tgt.dx = fp(22);   tgt.dy = fp(-8);
            tgt.lh = tgt.rh = fp(NEUT_H + 4);
            tgt.lcr = tgt.rcr = fp(NEUT_CR + 2);
            break;
        case SUB_THINKING:
            tgt.dx = fp(-15);  tgt.dy = fp(-10);
            ease_spd = EASE_SLOW;
            break;
        case SUB_WINK:
            tgt.lh = fp(2);
            break;
        default:
            break;
        }
        break;
    }

    /* ── SLEEPY ── fighting to stay awake ────── */
    case ST_SLEEPY: {
        tgt.lw  = tgt.rw  = fp(SLEEPY_W);
        tgt.lcr = tgt.rcr = fp(SLEEPY_CR);
        tgt.dy  = fp(2);

        /* alternating tired eyes: one droops, then the other
         * minimum height 8 keeps the pupil barely visible */
        int drowsy = frame % 300;            /* 5 s cycle */
        if (drowsy < 90) {
            tgt.lh = fp(SLEEPY_H + 8);      /* left fights open */
            tgt.rh = fp(8);                  /* right droops, pupil peeking */
        } else if (drowsy < 120) {
            tgt.lh = tgt.rh = fp(SLEEPY_H); /* both equal */
        } else if (drowsy < 210) {
            tgt.lh = fp(8);                  /* left droops, pupil peeking */
            tgt.rh = fp(SLEEPY_H + 8);      /* right fights open */
        } else {
            tgt.lh = tgt.rh = fp(SLEEPY_H); /* both settle low */
        }

        int dr = frame % 240;
        if      (dr < 80)  tgt.dx = 0;
        else if (dr < 160) tgt.dx = fp(-15);
        else               tgt.dx = fp(15);

        ease_spd   = EASE_SLOW;
        ease_h_spd = EASE_SLOW;
        break;
    }

    /* ── ASLEEP ── closed, breathing, zzz ─────── */
    case ST_ASLEEP: {
        tgt.lw  = tgt.rw  = fp(ASLEEP_W);
        tgt.lh  = tgt.rh  = fp(ASLEEP_H);
        tgt.lcr = tgt.rcr = fp(ASLEEP_CR);
        tgt.dx  = 0;

        /* breathing: ±1 px vertical over 3 s */
        int br = frame % 180;
        if (br < 90)
            tgt.dy = fp(-1) + br * fp(2) / 90;
        else
            tgt.dy = fp(1) - (br - 90) * fp(2) / 90;

        ease_spd   = EASE_GLACIAL;
        ease_h_spd = EASE_GLACIAL;
        break;
    }

    /* ── ANNOYED ── narrow, grumpy glare ──────── */
    case ST_ANNOYED: {
        tgt.lw  = tgt.rw  = fp(ANNOY_W);
        tgt.lh  = tgt.rh  = fp(ANNOY_H);
        tgt.lcr = tgt.rcr = fp(ANNOY_CR);
        tgt.dy  = 0;

        int gl = frame % 180;
        if      (gl < 60)  tgt.dx = 0;
        else if (gl < 120) tgt.dx = fp(-20);
        else               tgt.dx = fp(20);

        ease_spd = EASE_SLOW;
        break;
    }
    }

    /* ── Blink height override ────────────────── */
    if (is_blinking()) {
        tgt.lh = fp(2);
        if (!bl_wink) tgt.rh = fp(2);
    }
    /* Use fast height easing for the entire blink (close + open) */
    if (bl_phase != BL_IDLE) {
        ease_h_spd = (state == ST_SLEEPY) ? EASE_SLOW : EASE_FAST;
    }
}

/* ═══════════════════════════════════════════════════════════
 * Render
 * ═══════════════════════════════════════════════════════════ */

static void render(void)
{
    /* Ease all parameters toward targets */
    ease_toward(&cur.lw,  tgt.lw,  ease_spd);
    ease_toward(&cur.rw,  tgt.rw,  ease_spd);
    ease_toward(&cur.lh,  tgt.lh,  ease_h_spd);
    ease_toward(&cur.rh,  tgt.rh,  ease_h_spd);
    ease_toward(&cur.lcr, tgt.lcr, ease_spd);
    ease_toward(&cur.rcr, tgt.rcr, ease_spd);
    ease_toward(&cur.dx,  tgt.dx,  ease_spd);
    ease_toward(&cur.dy,  tgt.dy,  ease_spd);

    /* Clamp */
    if (cur.lw < fp(1))  cur.lw = fp(1);
    if (cur.rw < fp(1))  cur.rw = fp(1);
    if (cur.lh < fp(1))  cur.lh = fp(1);
    if (cur.rh < fp(1))  cur.rh = fp(1);
    if (cur.lcr < 0)     cur.lcr = 0;
    if (cur.rcr < 0)     cur.rcr = 0;

    /* Clear canvas */
    lv_canvas_fill_bg(canvas_obj, bg, LV_OPA_COVER);

    int gaze_x = unfp(cur.dx);
    int gaze_y = unfp(cur.dy);
    int lw = unfp(cur.lw), lh = unfp(cur.lh), lcr = unfp(cur.lcr);
    int rw = unfp(cur.rw), rh = unfp(cur.rh), rcr = unfp(cur.rcr);

    /* Parallax: gap between eyes compresses when looking sideways */
    int compress = (gaze_x > 0 ? gaze_x : -gaze_x) / 3;
    int ldx = gaze_x + compress;
    int rdx = gaze_x - compress;

    /* Eyes (with iris, highlight, thick lid + cat-eye wing, glow) */
    draw_eye(EYE_L_CX + ldx, EYE_CY + gaze_y, lw, lh, lcr, fg, gaze_x, gaze_y);
    draw_eye(EYE_R_CX + rdx, EYE_CY + gaze_y, rw, rh, rcr, fg, gaze_x, gaze_y);

    /* Nose dot (centered between eyes, slightly below) */
    draw_nose(gaze_y);

    /* Happy sparkle + joy marks (ramps up after sustained typing) */
    if (state == ST_HAPPY && sparkle_ramp > SPARKLE_DELAY && !is_blinking()) {
        draw_sparkle(EYE_L_CX + ldx, EYE_CY + gaze_y, lw, lh);
        draw_sparkle(EYE_R_CX + rdx, EYE_CY + gaze_y, rw, rh);
        draw_happy_marks(EYE_L_CX + ldx, EYE_CY + gaze_y, lh);
        draw_happy_marks(EYE_R_CX + rdx, EYE_CY + gaze_y, rh);
    }

    /* zzz when asleep */
    if (state == ST_ASLEEP)
        draw_zzz();

    lv_obj_invalidate(canvas_obj);
}

/* ═══════════════════════════════════════════════════════════
 * Animation loop
 * ═══════════════════════════════════════════════════════════ */

static void anim_handler(struct k_work *work)
{
    frame++;
    if (key_heat > 0) key_heat--;
    if (state == ST_HAPPY) {
        if (sparkle_ramp < 120) sparkle_ramp++;
    } else {
        if (sparkle_ramp > 0) sparkle_ramp -= 2;
    }
    update_state();
    update_sub();
    blink_tick();
    compute_targets();
    render();

    k_work_schedule_for_queue(
        zmk_display_work_q(), &anim_work, K_MSEC(TICK_MS));
}

/* ═══════════════════════════════════════════════════════════
 * ZMK key-event listener
 * ═══════════════════════════════════════════════════════════ */

static int emo_event_cb(const zmk_event_t *eh)
{
    const struct zmk_position_state_changed *ev =
        as_zmk_position_state_changed(eh);

    if (ev && ev->state) {                       /* key-down only */
        last_activity = k_uptime_get();
        key_heat += 4;

        if (state == ST_ASLEEP || state == ST_ANNOYED) {
            /* Wake up happy */
            set_state(ST_HAPPY);
            key_heat = HAPPY_HEAT_THRESHOLD;
        } else if (key_heat >= HAPPY_HEAT_THRESHOLD &&
                   state != ST_HAPPY) {
            set_state(ST_HAPPY);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(tamagotchi, emo_event_cb);
ZMK_SUBSCRIPTION(tamagotchi, zmk_position_state_changed);

/* ═══════════════════════════════════════════════════════════
 * Public init
 * ═══════════════════════════════════════════════════════════ */

void tamagotchi_widget_init(lv_obj_t *parent)
{
    bg = lv_color_black();
    fg = lv_color_white();

    rng_s = (uint32_t)k_uptime_get() ^ 0xDEADBEEFu;

    state         = ST_NEUTRAL;
    last_activity = k_uptime_get();
    frame         = 0;

    cur.lw = cur.rw = tgt.lw = tgt.rw = fp(NEUT_W);
    cur.lh = cur.rh = tgt.lh = tgt.rh = fp(NEUT_H);
    cur.lcr = cur.rcr = tgt.lcr = tgt.rcr = fp(NEUT_CR);
    cur.dx = cur.dy = tgt.dx = tgt.dy = 0;

    ease_spd   = EASE_NORMAL;
    ease_h_spd = EASE_NORMAL;

    sub_expr     = SUB_NONE;
    sub_timer    = 0;
    sub_cooldown = rng_range(300, 600);

    return_timer    = 0;
    grumpy_cooldown = 0;

    schedule_blink();

    memset(burst_ring, 0, sizeof(burst_ring));
    burst_idx     = 0;
    key_heat      = 0;
    sparkle_ramp  = 0;

    canvas_obj = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas_obj, cbuf,
                         DISP_W, DISP_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas_obj, LV_ALIGN_CENTER, 0, 0);

    k_work_init_delayable(&anim_work, anim_handler);
    k_work_schedule_for_queue(
        zmk_display_work_q(), &anim_work, K_NO_WAIT);
}
