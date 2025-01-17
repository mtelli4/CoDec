#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imgdif.h"

DifImg* create_difimg(int width, int height) {
    DifImg* dim = (DifImg*)malloc(sizeof(DifImg));
    if (!dim) return NULL;
    
    dim->width = width;
    dim->height = height;
    dim->img = (unsigned char*)calloc(width * height, sizeof(unsigned char));
    dim->dif = (dword*)calloc(width * height, sizeof(dword));
    
    if (!dim->img || !dim->dif) {
        free_difimg(dim);
        return NULL;
    }
    
    return dim;
}

void free_difimg(DifImg* dim) {
    if (dim) {
        free(dim->img);
        free(dim->dif);
        free(dim);
    }
}

void compute_differential(DifImg* dim, unsigned char* img) {
    int size = dim->width * dim->height;
    
    // Stocke le premier pixel
    dim->first = img[0];
    dim->img[0] = img[0];
    
    // Calcule les différences
    for (int i = 1; i < size; i++) {
        dim->dif[i] = (dword)img[i] - (dword)img[i-1];
        dim->img[i] = img[i];
    }
}

void reconstruct_image(DifImg* dim, unsigned char* img) {
    int size = dim->width * dim->height;
    
    // Premier pixel
    img[0] = dim->first;
    
    // Reconstruit à partir des différences
    for (int i = 1; i < size; i++) {
        img[i] = (unsigned char)(img[i-1] + dim->dif[i]);
    }
}

// Fonctions de gestion des fichiers PGM
void save_pgm(const char* filename, unsigned char* img, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    
    fprintf(f, "P5\n%d %d\n255\n", width, height);
    fwrite(img, width * height, 1, f);
    fclose(f);
}

unsigned char* load_pgm(const char* filename, int* width, int* height) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    char line[100];
    fgets(line, sizeof(line), f); // P5
    
    // Ignore les commentaires
    do {
        fgets(line, sizeof(line), f);
    } while (line[0] == '#');
    
    sscanf(line, "%d %d", width, height);
    fgets(line, sizeof(line), f); // 255
    
    unsigned char* img = (unsigned char*)malloc(*width * *height);
    if (!img) {
        fclose(f);
        return NULL;
    }
    
    fread(img, *width * *height, 1, f);
    fclose(f);
    
    return img;
}
