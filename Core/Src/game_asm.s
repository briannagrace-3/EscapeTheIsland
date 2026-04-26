.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.section .text
.balign 2

/* ─────────────────────────────────────────────
   int8_t clamp(int32_t val, int32_t min, int32_t max)
   r0 = val, r1 = min, r2 = max
   returns clamped value in r0
   ───────────────────────────────────────────── */
.global clamp
clamp:
    CMP     r0, r1          //if val < min
    IT      LT
    MOVLT   r0, r1          // r0 = min
    CMP     r0, r2          // if val > max
    IT      GT
    MOVGT   r0, r2          // r0 = max
    BX      LR              // return

/* ─────────────────────────────────────────────
   Pseudo-random number generator:
   uint32_t asm_rng(uint32_t seed)
   r0 = seed
   returns next LCG value in r0
   LCG: seed = seed * 1664525 + 1013904223
   ───────────────────────────────────────────── */
.global asm_rng
asm_rng:
    LDR     r1, =1664525        //multiplier
    MUL     r0, r0, r1          //seed * multiplier
    LDR     r1, =1013904223     // increment
    ADD     r0, r0, r1          // increment
    BX      LR                  //return
