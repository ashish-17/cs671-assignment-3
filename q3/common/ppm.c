#include <stdlib.h>
#include <stdio.h>
#include <math.h>

float min(float X, float Y) { return X < Y ? X : Y;}

void writePPMImage(int* data, int width, int height, const char *filename, int maxIterations)
{
    FILE *fp = fopen(filename, "wb");

    // write ppm header
    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", width, height);
    fprintf(fp, "255\n");

    int i = 0, j = 0;
    float mapped = 0.0;
    unsigned char result;
    for (i = 0; i < width*height; ++i) {

        // Clamp iteration count for this pixel, then scale the value
        // to 0-1 range.  Raise resulting value to a power (<1) to
        // increase brightness of low iteration count
        // pixels. a.k.a. Make things look cooler.

        mapped = pow(min((float)(maxIterations), (float)(data[i])) / 256.f, .5f);

        // convert back into 0-255 range, 8-bit channels
        result = (unsigned char)(255.f * mapped);
        for (j = 0; j < 3; ++j)
            fputc(result, fp);
    }

    fclose(fp);
    printf("Wrote image file %s\n", filename);
}
