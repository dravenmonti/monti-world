// Monti Noise, useful for terrain generation
// Authored by Draven Monti
// Uses https://github.com/nothings/stb for image rendering

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Convert two coordinates into a hash using xorshift
uint64_t hash(uint64_t x, uint64_t y) {
	x ^= x << 13 + y << 13;
	y ^= x >> 7 + y >> 7;
	x ^= x << 17 + y << 17;	
	y ^= x << 13 + y << 13;
	x ^= x >> 7 + y >> 7;
	y ^= x << 17 + y << 17;
	return x;
}

// Convert coordinate hash to grid, and interpolate grid with zoomed + rotated hash (stronger at edges) 
double monti(double x, double y, double depth, int seed) {
	int xI = floor(x), yI = floor(y);
	uint64_t h = hash(xI*7919+yI*7907+seed,yI*6277-xI*6053+seed);

	double out = (h % 6700417) / 6700417.0;

	if (depth == 0) return 0.5;

	double dist = (0.5 - fabs(x - xI - 0.5)) * (0.5 - fabs(y - yI - 0.5)) * 4;
	dist *= 0.9;

	out = out * dist + monti(x * 1.5 + y * 0.2, y * 1.5 - x * 0.2, depth - 1, seed+1) * (1.0 - dist);

	return out;
}

// Convert integer to string, useful for parsing command line arguments
int to_int(char *buf) {
    int i = 0, x = 0;
    for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
        i = i * 10 + buf[x] - '0';
    }
    return i;
}

// Parse command line and generate image
int main(int argc, char** argv) {
	if (argc < 4) {
		printf("%s [width] [height] [output file]\n",argv[0]);
		return 1;
	}

	int width = to_int(argv[1]), height = to_int(argv[2]);
	unsigned char *data = malloc(width*height*4);

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int c = i*width*4 + j*4;

			double val = monti(i / 400.0,j / 400.0,15,0);

			data[c] = data[c+1] = data[c+2] = ((val > 0.7) ? 255 : 0) * 0.7 + val * 255 * 0.3;
			data[c+3] = 255;
		}
		printf("rendered %i / %i\n",i,height);
	}

	stbi_write_png(argv[3],width,height,4,data,width*4);

	free(data);
}
