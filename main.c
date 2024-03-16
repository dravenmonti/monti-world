// Extension of Monti Noise
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
double monti_height(double x, double y, int depth, int seed) {
	int x_i = floor(x), y_i = floor(y);
	uint64_t h = hash(x_i*7919+y_i*7907+seed,y_i*6277-x_i*6053+seed);

	double out = (h % 6700417) / 6700417.0;

	if (depth == 0) return 0.5;

	double dist = (0.5 - fabs(x - x_i - 0.5)) * (0.5 - fabs(y - y_i - 0.5)) * 4;
	dist *= 0.9;

	out = out * dist + monti_height(x * 1.5 + y * 0.2, y * 1.5 - x * 0.2, depth - 1, seed+1) * (1.0 - dist);

	return out;
}

// Rivers going down a height gradient
double monti_river(double x, double y, int depth, int seed) {
	double p_0_0 = monti_height(x,y,depth,seed),
	       p_0_1 = monti_height(x + 0.01,y + 0.01,depth,seed),
	       p_1_0 = monti_height(x,y + .5,depth,seed),
	       p_1_1 = monti_height(x + 0.01,y + 0.01,depth,seed),
	       river = monti_height(x/1.5,y/1.5,depth/2,seed+69);

	double grad_x = (p_0_0 + p_0_1) - (p_1_0 + p_1_1);
	double grad_y = (p_0_0 + p_1_0) - (p_0_1 + p_1_1);

	double grad_d = sqrt(grad_x * grad_x + grad_y * grad_y) * 0.5 - 0.1;

	double smt = fabs(river - 0.5);

	smt *= 3;

	return (smt < grad_d) ? (smt / grad_d) : 1;
}

// Convert integer to string, useful for parsing command line arguments
int to_int(char *buf) {
    int i = 0, x = 0;
    for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
        i = i * 10 + buf[x] - '0';
    }
    return i;
}

// Convert double to string
double to_double(char *buf) {
    int i = 0, x = 0;
    for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
        i = i * 10 + buf[x] - '0';
    }
	if (buf[x] != '.') {
		return i;
	}
	x++;
	double place = 1;
	for (; buf[x] >= '0' && buf[x] <= '9'; x++) {
        place *= 0.1;
		i = i * 10 + buf[x] - '0';
    }
	
	return i * place;
}

// Parse command line and generate image
int main(int argc, char** argv) {
	if (argc < 7) {
		printf("%s [width] [height] [scale] [seed] [mode] [output file]\n",argv[0]);
		return 1;
	}

	int width = to_int(argv[1]), height = to_int(argv[2]), 
		seed = to_int(argv[4]), mode = to_int(argv[5]);
	double scale = to_double(argv[3]);
	unsigned char *data = malloc(width*height*4);

	// This will probably need to be refactored soon.
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int c = i*width*4 + j*4;

			double x = i / 3000.0 * scale, y = j / 3000.0 * scale;

			double val = monti_height(x,y,18,seed);
			int isRiver = 0;
			if (val > 0.58) {
				double river = monti_river(x,y,18,seed);
				val = val - 0.2 + 0.2 * river;
				isRiver = river < 0.85;
			}
			if (val > 0.99) val = 0.99;

			data[c+3] = 255;
	
			if (mode == 1) {
				data[c] = val * 255;

				// Biome
				data[c+1] |= (!isRiver && val > 0.6) << 7;
				data[c+1] |= (isRiver && val > 0.6) << 6;
				data[c+1] |= (!isRiver && val > 0.6 && val < 0.64) << 5;

				continue;
			}

			data[c] = (val > 0.6 && val < 0.64 && !isRiver) ? 255 : val;
			data[c+1] = ((val > 0.6 && !isRiver) ? 255 : 0) * 0.7 + val * 255 * 0.3;
			data[c+2] = ((val > 0.6 && !isRiver) ? 0 : 255) * 0.3 + val * 255 * 0.7;

			double dec = (((int) (val * 256.0)) % 2) ? 1.0 : 0.9;

			data[c] *= dec;
			data[c+1] *= dec;
			data[c+2] *= dec;
		}
		printf("rendered %i / %i\n",i,height);
	}

	stbi_write_png(argv[6],width,height,4,data,width*4);

	free(data);
}
