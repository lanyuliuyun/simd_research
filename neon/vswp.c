
/* 
arm-linux-gnueabihf-gcc -mcpu=cortex-a7 -mfpu=neon-vfpv4 -O2 ./vswp.c -o ./vswp
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int64_t ts_us(void)
{
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);

    return (int64_t)tspec.tv_sec * 100000 + (int64_t)tspec.tv_nsec / 1000;
}

void rgb2bgr(uint8_t *out, const uint8_t *in, int w, int h)
{
    int x, y;
    const uint8_t *line_in = in;
    uint8_t *line_out = out;
    int bytes_per_px = 3;
    int bytes_per_line = w * bytes_per_px;
    int w_align = (w & ~15);

    for (y = 0; y < h; ++y)
    {
        //for (x = 0; x < w; x+=8)
        for (x = 0; x < (w & ~7); x+=16)
        {
            const uint8_t *pt_in = line_in + (x * bytes_per_px);
            uint8_t *pt_out = line_out + (x * bytes_per_px);
            asm volatile(
              "vld3.8 {d0, d1, d2}, [%1]!    \n\t"
              "vswp d0, d2                   \n\t"
              "vst3.8 {d0, d1, d2}, [%0]!     \n\t"
             : "+r"(pt_out)         /* %0 */
             : "r"(pt_in)           /* %1 */
             : "memory", "d0", "d1", "d2"
            );
        }
        line_in += bytes_per_line;
        line_out += bytes_per_line;
    }
    if (w_align < w)
    {
        line_in = in;
        line_out = out;
        for (y = 0; y < h; ++y)
        {
            for (x = w_align; x < w; x++)
            {
                const uint8_t *pt_in = line_in + (x * bytes_per_px);
                uint8_t *pt_out = line_out + (x * bytes_per_px);
                pt_out[0] = pt_in[2];
                pt_out[2] = pt_in[0];
            }
            line_in += bytes_per_line;
            line_out += bytes_per_line;
        }
    }
}

int main(int argc, char *argv[])
{
    int64_t ts, delta;
    
    int img_size = 1080*1920*3;
    uint8_t *img_buffer = (uint8_t *)malloc(img_size * 2);
    uint8_t *img_in = img_buffer;
    uint8_t *img_out = img_buffer + img_size;

    FILE *fp_in = fopen("/root/color-1080x1920.bgr24", "r");
    FILE *fp_out = fopen("/root/color-1080x1920.rgb24", "w");
    fread(img_in, 1, img_size, fp_in);

    ts = ts_us();
    rgb2bgr(img_out, img_in, 1080, 1920);
    delta = ts_us() - ts;
    printf("elapsed: %" PRId64 "us\n", delta);

    fwrite(img_out, 1, img_size, fp_out);
    free(img_buffer);

    return 0;
}