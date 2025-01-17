#include <stdio.h>
#include <stdlib.h>
#include "codec.h"

// Utilitaires de manipulation de bits
static uint8_t bit_buffer = 0;
static int bit_count = 0;

static void push_bits(uint8_t** buffer, size_t* pos, uint32_t bits, int count) {
	while (count > 0) {
		bit_buffer = (bit_buffer << 1) | ((bits >> (count - 1)) & 1);
		bit_count++;
		
		if (bit_count == 8) {
			(*buffer)[(*pos)++] = bit_buffer;
			bit_buffer = 0;
			bit_count = 0;
		}
		count--;
	}
}

static uint32_t pull_bits(uint8_t* buffer, size_t* pos, int count) {
	uint32_t result = 0;
	while (count > 0) {
		if (bit_count == 0) {
			bit_buffer = buffer[(*pos)++];
			bit_count = 8;
		}
		
		result = (result << 1) | ((bit_buffer >> (bit_count - 1)) & 1);
		bit_count--;
		count--;
	}
	return result;
}

void init_quantizer(Quantizer* q) {
	q->num_levels = NUM_LEVELS;
	
	// Niveau 0: [0, 2[
	q->levels[0].nbits = 1;
	q->levels[0].prefix = 0;
	q->levels[0].preflen = 1;
	q->levels[0].offset = 0;
	q->levels[0].min = 0;
	q->levels[0].max = 2;
	
	// Niveau 1: [2, 6[
	q->levels[1].nbits = 2;
	q->levels[1].prefix = 2;
	q->levels[1].preflen = 2;
	q->levels[1].offset = 2;
	q->levels[1].min = 2;
	q->levels[1].max = 6;
	
	// Niveau 2: [6, 22[
	q->levels[2].nbits = 4;
	q->levels[2].prefix = 6;
	q->levels[2].preflen = 3;
	q->levels[2].offset = 6;
	q->levels[2].min = 6;
	q->levels[2].max = 22;
	
	// Niveau 3: [22, 256[
	q->levels[3].nbits = 8;
	q->levels[3].prefix = 7;
	q->levels[3].preflen = 3;
	q->levels[3].offset = 22;
	q->levels[3].min = 22;
	q->levels[3].max = 256;
}

static QuantLevel* find_level(Quantizer* q, int16_t value) {
	value = abs(value);
	for (int i = 0; i < q->num_levels; i++) {
		if (value >= q->levels[i].min && value < q->levels[i].max) {
			return &q->levels[i];
		}
	}
	return &q->levels[q->num_levels - 1];
}

int compress_data(DifImg* dim, uint8_t** buffer, size_t* bufsize) {
	Quantizer q;
	init_quantizer(&q);
	
	size_t size = dim->width * dim->height;
	*bufsize = size * 2; // Pire des cas
	*buffer = (uint8_t*)malloc(*bufsize);
	if (!*buffer) return 0;
	
	size_t pos = 0;
	bit_buffer = 0;
	bit_count = 0;
	
	for (size_t i = 1; i < size; i++) {
		dword diff = dim->dif[i];
		QuantLevel* level = find_level(&q, diff);
		
		// Pousse le préfixe
		push_bits(buffer, &pos, level->prefix, level->preflen);
		
		// Pousse la valeur absolue moins le décalage
		int16_t abs_val = abs(diff) - level->offset;
		push_bits(buffer, &pos, abs_val, level->nbits);
		
		// Pousse le signe (0 pour positif, 1 pour négatif)
		push_bits(buffer, &pos, diff < 0 ? 1 : 0, 1);
	}
	
	// Vide les bits restants
	if (bit_count > 0) {
		(*buffer)[pos++] = bit_buffer << (8 - bit_count);
	}
	
	*bufsize = pos;
	return 1;
}

int decompress_data(uint8_t* buffer, size_t bufsize, DifImg* dim) {
	Quantizer q;
	init_quantizer(&q);
	
	size_t pos = 0;
	bit_buffer = 0;
	bit_count = 0;
	size_t size = dim->width * dim->height;
	
	for (size_t i = 1; i < size && pos < bufsize; i++) {
		// Lit le préfixe
		uint32_t prefix = 0;
		int found = 0;
		
		for (int j = 1; j <= 3 && !found; j++) {
			prefix = (prefix << 1) | pull_bits(buffer, &pos, 1);
			
			for (int k = 0; k < q.num_levels; k++) {
				if (q.levels[k].preflen == j && q.levels[k].prefix == prefix) {
					// Niveau correspondant trouvé
					uint32_t abs_val = pull_bits(buffer, &pos, q.levels[k].nbits);
					uint32_t sign = pull_bits(buffer, &pos, 1);
					
					abs_val += q.levels[k].offset;
					dim->dif[i] = sign ? -abs_val : abs_val;
					found = 1;
					break;
				}
			}
		}
		
		if (!found) return 0;
	}
	
	return 1;
}