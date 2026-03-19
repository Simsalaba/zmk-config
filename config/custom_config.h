// Copyright 2021 Manna Harbour
// https://github.com/manna-harbour/miryoku

/*
 * Use QWERTY Miryoku while keeping the stock Corne mapping and behavior model
 * intact. Only the base layer mapping is extended to use the right outer
 * column for the Swedish letters.
 *
 * The dedicated Swedish-letter keys below are intended for a macOS
 * "ABC - Extended" host input source.
 */
#define MIRYOKU_ALPHAS_QWERTY
#define MIRYOKU_TAP_QWERTY

#define MIRYOKU_LAYERMAPPING_BASE MIRYOKU_LAYOUTMAPPING_CORNE_SE

#define MIRYOKU_LAYOUTMAPPING_CORNE_SE( \
     K00, K01, K02, K03, K04,      K05, K06, K07, K08, K09, \
     K10, K11, K12, K13, K14,      K15, K16, K17, K18, K19, \
     K20, K21, K22, K23, K24,      K25, K26, K27, K28, K29, \
     N30, N31, K32, K33, K34,      K35, K36, K37, N38, N39 \
) \
&none K00 K01 K02 K03 K04          K05 K06 K07 K08 K09 &sw_aring       \
&none K10 K11 K12 K13 K14          K15 K16 K17 K18 K19 &sw_odiaeresis  \
&none K20 K21 K22 K23 K24          K25 K26 K27 K28 K29 &sw_adiaeresis  \
               K32 K33 K34         K35 &u_lt_bspc_rep U_NUM BSPC K37
