#ifndef __IMGDIF_H
#define __IMGDIF_H

#include <stdint.h>

typedef int16_t dword;  // pour les valeurs différentielles

typedef struct {
	unsigned char* img;  // données de l'image originale
	dword* dif;         // données différentielles
	int width;
	int height;
	unsigned char first; // valeur du premier pixel
} DifImg;

// Prototypes des fonctions
DifImg* create_difimg(int width, int height);
void free_difimg(DifImg* dim);
void compute_differential(DifImg* dim, unsigned char* img);
void reconstruct_image(DifImg* dim, unsigned char* img);
void save_pgm(const char* filename, unsigned char* img, int width, int height);
unsigned char* load_pgm(const char* filename, int* width, int* height);

#endif