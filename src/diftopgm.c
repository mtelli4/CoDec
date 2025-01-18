#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <g2x.h>
#include "imgdif.h"
#include "codec.h"
#include <g2x_draw.h>
#include <g2x_control.h>
#include <g2x_window.h>  
#include <GL/glut.h>
#include <GL/gl.h>

// Variables globales pour l'interface graphique
static DifImg* dim = NULL;
static unsigned char* display_img = NULL;
static bool show_differential = false;
static bool show_histogram = false;
extern void g2x_Redraw(void);
extern void g2x_Clear(void);
extern void g2x_WriteString(double x, double y, const char* text);

// Callbacks pour l'interface graphique
void toggle_differential(void) {
    show_differential = !show_differential;
    g2x_Redraw();
}

void toggle_histogram(void) {
    show_histogram = !show_histogram;
    g2x_Redraw();
}

// Fonction d'affichage principale
void display_func(void) {
    if (!dim || !display_img) return;
    
    // Effacement de l'écran
    g2x_Clear();
    
    int size = dim->width * dim->height;

    // Préparation de l'image à afficher
    if (show_differential) {
        for (int i = 0; i < size; i++) {
            display_img[i] = (unsigned char)(128 + dim->dif[i] / 2);
        }
    } else {
        reconstruct_image(dim, display_img);
    }

    // Même logique d'affichage que dans pgmtodif.c
    double img_width = g2x_GetXMax() - g2x_GetXMin();
    double img_height = g2x_GetYMax() - g2x_GetYMin();
    if (show_histogram) img_height *= 0.7;

    double scale_x = img_width / dim->width;
    double scale_y = img_height / dim->height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;

    double start_x = (img_width - (dim->width * scale)) / 2 + g2x_GetXMin();
    double start_y = (img_height - (dim->height * scale)) / 2 + g2x_GetYMin();

    // Affichage de l'image
    for (int y = 0; y < dim->height; y++) {
        for (int x = 0; x < dim->width; x++) {
            unsigned char pixel = display_img[y * dim->width + x];
            double gray = pixel / 255.0;
            G2Xcolor color = {gray, gray, gray, 1.0};
            
            g2x_FillRectangle(
                start_x + x * scale,
                start_y + y * scale,
                start_x + (x + 1) * scale,
                start_y + (y + 1) * scale,
                color
            );
        }
    }

    // Affichage de l'histogramme si demandé
    if (show_histogram) {
        int histogram[256] = {0};
        int max_count = 0;

        for (int i = 0; i < size; i++) {
            histogram[display_img[i]]++;
            if (histogram[display_img[i]] > max_count) {
                max_count = histogram[display_img[i]];
            }
        }

        double hist_y = g2x_GetYMin();
        double hist_height = g2x_GetYMax() * 0.2;
        double hist_width = g2x_GetXMax() - g2x_GetXMin();
        double bar_width = hist_width / 256.0;

        G2Xcolor hist_color = {0.5, 0.5, 0.5, 1.0};
        for (int i = 0; i < 256; i++) {
            double height = (histogram[i] * hist_height) / max_count;
            g2x_FillRectangle(
                g2x_GetXMin() + i * bar_width,
                hist_y,
                g2x_GetXMin() + (i + 0.8) * bar_width,
                hist_y + height,
                hist_color
            );
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <input.dif>\n", argv[0]);
        return 1;
    }

    // Lecture du fichier DIFF
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        printf("Cannot open file %s\n", argv[1]);
        return 1;
    }

    // Lecture de l'en-tête
    uint16_t magic, width, height;
    uint8_t num_levels;
    uint8_t level_bits[4];
    unsigned char first_pixel;

    fread(&magic, sizeof(magic), 1, f);
    if (magic != MAGIC_NUMBER) {
        printf("Invalid DIFF file\n");
        fclose(f);
        return 1;
    }

    fread(&width, sizeof(width), 1, f);
    fread(&height, sizeof(height), 1, f);
    fread(&num_levels, sizeof(num_levels), 1, f);
    fread(level_bits, sizeof(level_bits), 1, f);
    fread(&first_pixel, sizeof(first_pixel), 1, f);

    // Création de l'image différentielle
    dim = create_difimg(width, height);
    if (!dim) {
        printf("Failed to create differential image\n");
        fclose(f);
        return 1;
    }

    dim->first = first_pixel;

    // Lecture des données compressées
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    size_t data_size = file_size - 11; // Taille totale - taille de l'en-tête
    fseek(f, 11, SEEK_SET); // Retour après l'en-tête

    uint8_t* buffer = (uint8_t*)malloc(data_size);
    if (!buffer) {
        printf("Failed to allocate buffer\n");
        free_difimg(dim);
        fclose(f);
        return 1;
    }

    fread(buffer, data_size, 1, f);
    fclose(f);

    // Décompression
    if (!decompress_data(buffer, data_size, dim)) {
        printf("Decompression failed\n");
        free(buffer);
        free_difimg(dim);
        return 1;
    }

    free(buffer);

    // Allocation du buffer d'affichage
    display_img = (unsigned char*)malloc(width * height);
    if (!display_img) {
        printf("Failed to allocate display buffer\n");
        free_difimg(dim);
        return 1;
    }

    // Sauvegarde de l'image reconstruite
    reconstruct_image(dim, display_img);
    
    char output_file[256];
    const char* basename = strrchr(argv[1], '/') ? strrchr(argv[1], '/') + 1 : argv[1];
    snprintf(output_file, sizeof(output_file), "./PGM/%s.pgm", basename);
		save_pgm(output_file, display_img, width, height);

    // Configuration de l'interface graphique
    g2x_InitWindow("Image Decoder", width, height);
    
    // Ajout des contrôles
		g2x_CreateSwitch("Show Differential", &show_differential, "Toggle differential view");
		g2x_CreateSwitch("Show Histogram", &show_histogram, "Toggle histogram view");
    
    // Configuration des fonctions de callback
    glutDisplayFunc(display_func);    
    // Lancement de la boucle principale
    return g2x_MainStart();
}