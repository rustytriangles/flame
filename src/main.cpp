#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/vreg.h"

#include "hub75.hpp"

const uint32_t GRID_WIDTH  = 128;
const uint32_t GRID_HEIGHT =  64;

Hub75 hub75(GRID_WIDTH, GRID_HEIGHT, nullptr, PANEL_GENERIC, true);

unsigned char grid[GRID_WIDTH][GRID_HEIGHT];

Pixel colormap[256];

void clear_grid() {
    for (int i=0; i<GRID_WIDTH; i++) {
        for (int j=0; j<GRID_HEIGHT; j++) {
            grid[i][j] = 0;
        }
    }
}

void init_colormap() {
    for (int i=0; i<256; i++) {
        const float f = (float)i / 255.f;
        colormap[i] = hsv_to_rgb(f / 3.f, 1.f, (1.f + 2.f * f) / 3.f);
    }
}

void rand_init() {
    std::srand(std::time(nullptr));
    for (int i=0; i<15; i++) {
        (void)std::rand();
    }
}

float random_between(float minval, float maxval) {
    float v = (float)std::rand() / (float)RAND_MAX;
    return minval + v*(maxval - minval);
}

void __isr dma_complete() {
    hub75.dma_complete();
}

int main() {

    stdio_init_all();

    hub75.start(dma_complete);
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(100);
    set_sys_clock_khz(250000, false);

    init_colormap();
    rand_init();

    clear_grid();
    for (int i=0; i<GRID_HEIGHT; i++) {
        grid[0][i] = (unsigned char)random_between(0.f,255.f);
    }

    const float mult = 1.f / (4.f + 1.e-5f);

    while (true) {

        hub75.clear();

        for (int i=0; i<GRID_HEIGHT; i++) {
            float new_v = (float)grid[0][i] + random_between(-15.f,15.f);
            grid[0][i] = (unsigned char)std::max(0.f,std::min(255.f,new_v));
        }

        float minval = 1000.f;
        float maxval = -1000.f;
        for (uint32_t x=GRID_WIDTH-1; x>0; x--) {
            for (uint32_t y=0; y<GRID_HEIGHT; y++) {
                unsigned char v1 =       grid[x-1][y];
                unsigned char v2 =       grid[x-1][(y+1)%GRID_HEIGHT];
                unsigned char v3 =       grid[x-1][(y+GRID_HEIGHT-1)%GRID_HEIGHT];
                unsigned char v4 = (x>1)?grid[x-2][y]:v1;

                float v = ( (float)v1 + (float)v2 + (float)v3 + (float)v4 ) * mult;
                minval = std::min(minval,v);
                maxval = std::max(maxval,v);
                grid[x][y] = (unsigned char)(v);
            }
        }

        for (uint32_t x=0; x<GRID_WIDTH; x++) {
            for (uint32_t y=0; y<GRID_HEIGHT; y++) {
                hub75.set_color(x,y,colormap[grid[x][y]]);
            }
        }

        printf("flip range = %g,%g\n",minval,maxval);

        hub75.flip(true); // Flip and clear to the background colour

        sleep_ms(1);
    }
}

