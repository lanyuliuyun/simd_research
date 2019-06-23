
/*
arm-sunxiA20-linux-gnueabi-gcc -mcpu=cortex-a7 -mfpu=neon-vfpv4 -ggdb -ftree-vectorize ./neon.c -o neon
*/

#include <arm_neon.h>
#include <time.h>
#include <inttypes.h>
#include <stdio.h>

int64_t ts_ns(void)
{
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);

    return (int64_t)tspec.tv_sec * 100000000 + tspec.tv_nsec;
}

void add_sum_c(int *a, const int *b, const int *c, int count)
{
    int i;
    for (i = 0; i < count; i+=4)
    {
        a[i] = b[i] + c[i];
    }

    return;
}

void add_sum_intrinsics(int *a, const int *b, const int *c, int count)
{
    int32x4_t v1,v2,v3;
    int i;

    for (i = 0; i < (count & ~3); i+=4)
    {
        v1 = vld1q_s32(b+i);
        v2 = vld1q_s32(c+i);
        v3 = vaddq_s32(v1, v2);
        vst1q_s32(a+i, v3);
    }

    return;
}

void add_sum_asm(int *a, const int *b, const int *c, int count)
{
  asm volatile(
    "1:                          \n\t"
      "vld1.32 {q1}, [%1]!       \n\t"
      "vld1.32 {q2}, [%2]!       \n\t"
      "vadd.s32 q0, q1, q2       \n\t"
      "vst1.32 {q0}, [%0]!       \n\t"
      "subs %3, %3, #4           \n\t"
      "bgt 1b                    \n\t"
    : "+r"(a)      /* %0 */
    : "r"(b),      /* %1 */
      "r"(c),      /* %2 */
      "r"(count)   /* %3 */
    : "memory", "q0", "q1", "q2"
    );
}

void test_add_sum(void)
{
    int a[1024];
    int b[1024];
    int c[1024];
    int i;
    
    int64_t ts0, delta0, delta1, delta2;

    for (i = 0; i < 1024; ++i)
    {
        a[i] = 0;
        b[i] = i;
        c[i] = i+1;
    }
    ts0 = ts_ns();
    add_sum_c(a, b, c, 1024);
    delta0 = ts_ns() - ts0;
    
    for (i = 0; i < 1024; ++i)
    {
        a[i] = 0;
        b[i] = i;
        c[i] = i+1;
    }
    ts0 = ts_ns();
    add_sum_intrinsics(a, b, c, 1024);
    delta1 = ts_ns() - ts0;

    for (i = 0; i < 1024; ++i)
    {
        a[i] = 0;
        b[i] = i;
        c[i] = i+1;
    }
    ts0 = ts_ns();
    add_sum_asm(a, b, c, 1024);
    delta2 = ts_ns() - ts0;
    
    printf(
      "add_sum_c: elapse: %" PRId64 "ns\n"
      "add_sum_intrinsics: elapse: %" PRId64 "ns\n"
      "add_sum_asm: elapse: %" PRId64 "ns\n", 
      delta0, delta1, delta2);
}

void test_addl(void)
{
    uint16_t a[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    const uint8_t b[8] = {200, 200, 200, 200, 200, 200, 200, 200};
    const uint8_t c[8] = {250, 250, 250, 250, 250, 250, 250, 250};

    uint16_t *pa = a;
    const uint8_t *pb = b;
    const uint8_t *pc = c;
    asm volatile(
        "vld1.8 {d0}, [%1]    \n\t"
        "vld1.8 {d1}, [%2]    \n\t"
        "vaddl.u8 q1, d0, d1  \n\t"
        "vst1.16 {q1}, [%0]    \n\t"
      : "+r"(pa)         /* %0 */
      : "r"(pb),         /* %1 */
        "r"(pc)          /* %2 */
      : "memory", "d0", "d1", "q1"
    );
    
    printf(
     "b: %d, %d, %d, %d, %d, %d, %d, %d\n"
     "c: %d, %d, %d, %d, %d, %d, %d, %d\n"
     "a: %d, %d, %d, %d, %d, %d, %d, %d\n", 
     b[0], b[1],b[2],b[3],b[4],b[5],b[6],b[7],
     c[0], c[1],c[2],c[3],c[4],c[5],c[6],c[7],
     a[0], a[1],a[2],a[3],a[4],a[5],a[6],a[7]
    );
}

void test_addw(void)
{
    uint16_t a[8] = {250, 250, 250, 250, 250, 250, 250, 250};
    uint8_t b[8] = {200, 200, 200, 200, 200, 200, 200, 200};

    uint16_t *pa = a;
    uint8_t *pb = b;
    asm volatile(
        "vld1.16 {q0}, [%0]    \n\t"
        "vld1.8 {d2}, [%1]     \n\t"
        "vaddw.u8 q0, q0, d2   \n\t"
        "vst1.16 {q0}, [%0]    \n\t"
      : "+r"(pa)         /* %0 */
      : "r"(pb)          /* %1 */
      : "memory", "d0", "d1", "q1"
    );
    
    printf(
     "b: %d, %d, %d, %d, %d, %d, %d, %d\n"
     "a: %d, %d, %d, %d, %d, %d, %d, %d\n", 
     b[0], b[1],b[2],b[3],b[4],b[5],b[6],b[7],
     a[0], a[1],a[2],a[3],a[4],a[5],a[6],a[7]
    );
}

void test_fast_sum(void)
{
    uint32_t y_sum = 0, v1, v2;
    uint16_t ydata[600][800];
    int x,y;
    uint16_t *line, *pt;
    int64_t ts0, delta0;

    for (y = 0; y < 600; ++y)
    {
        for (x = 0; x < 800; ++x)
        {
            ydata[y][x] = 1;
        }
    }

    ts0 = ts_ns();
    line = &ydata[0][0];
    for (y = 0; y < 600; ++y)
    {
        for (x = 0; x < 800; x+=16)
        {
            pt = line + x;
            asm volatile(
              "vld2.8 {q0,q1}, [%3]      \n\t"
              "vaddl.u8 q1, d0, d1       \n\t"
              "vaddl.u16 q2, d2, d3      \n\t"
              "vpadd.u32 d6, d4, d5      \n\t"
              "vmov.u32 %1, %2, d6       \n\t"
              "add %0, %0, %1            \n\t"
              "add %0, %0, %2            \n\t"
            : "+r"(y_sum),           /* %0 */
              "+r"(v1),              /* %1 */
              "+r"(v2)               /* %2 */
            : "r"(pt)                /* %3 */
            : "cc", "memory", "q0", "q1", "q2", "q3"
            );
        }
        line += 800;
    }
    delta0 = ts_ns() - ts0;
    printf("test_fast_sum, y_sum: %" PRIu32 ", elapse: %" PRIu64 "ns\n", y_sum, delta0);

    return;
}

void test_c_sum(void)
{
    uint32_t y_sum = 0;
    uint16_t ydata[600][800];
    int x,y;
    int64_t ts0, delta0;

    for (y = 0; y < 600; ++y)
    {
        for (x = 0; x < 800; ++x)
        {
            ydata[y][x] = 1;
        }
    }

    ts0 = ts_ns();
    y_sum = 0;
    for (y = 0; y < 600; ++y)
    {
        for (x = 0; x < 800; ++x)
        {
            y_sum += ydata[y][x] & 0x00FF;
        }
    }
    delta0 = ts_ns() - ts0;

    printf("test_c_sum, y_sum: %" PRIu32 ", elapse: %" PRIu64 "ns\n", y_sum, delta0);
    return;
}

int main(int argc, char *argv[])
{
    test_add_sum();
    
    test_addl();
    
    test_addw();
    
    test_fast_sum();
    test_c_sum();

    return 0;
}
