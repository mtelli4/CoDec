#ifndef __CODEC_H
#define __CODEC_H

#include "imgdif.h"

#define MAGIC_NUMBER 0xD1FF
#define NUM_LEVELS 4

typedef struct {
	uint8_t nbits;   // nombre de bits pour ce niveau
	uint8_t prefix;  // préfixe VLC
	uint8_t preflen; // longueur du préfixe en bits
	int16_t offset;  // valeur de décalage
	int16_t min;     // valeur minimale de la plage
	int16_t max;     // valeur maximale de la plage
} QuantLevel;

typedef struct {
	QuantLevel levels[NUM_LEVELS];
	int num_levels;
} Quantizer;

// Prototypes des fonctions
int encode_image(const char* input_file, const char* output_file);
int decode_image(const char* input_file, const char* output_file);
void init_quantizer(Quantizer* q);
int compress_data(DifImg* dim, uint8_t** buffer, size_t* bufsize);
int decompress_data(uint8_t* buffer, size_t bufsize, DifImg* dim);

#endif