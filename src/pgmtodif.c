#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <g2x.h>
#include "imgdif.h"
#include "codec.h"
#include <g2x_draw.h>
#include <g2x_control.h>
#include <g2x_window.h>  
#include <GL/freeglut.h>

// Variables globales pour l'interface graphique
static DifImg* dim = NULL;
static unsigned char* display_img = NULL;
static bool show_differential = false;
static bool show_histogram = false;
static float compression_rate = 0.0;
extern bool g2x_SetDisplayFunc(void (*f)(void));
extern void g2x_Redraw(void);
extern void g2x_Clear(void);
extern void g2x_WriteString(double x, double y, const char* text);

// Callbacks pour l'interface graphique
static void toggle_differential(void) {
    show_differential = !show_differential;
    glutPostRedisplay();
}

static void toggle_histogram(void) {
    show_histogram = !show_histogram;
    glutPostRedisplay();
}


// Fonction d'affichage principale
void display_func(void) {
    if (!dim) return;
    
    // Effacement de l'écran
		glClear(GL_COLOR_BUFFER_BIT);

    int size = dim->width * dim->height;
    if (!display_img) return;

    // Préparation de l'image à afficher
    if (show_differential) {
        for (int i = 0; i < size; i++) {
            display_img[i] = (unsigned char)(128 + dim->dif[i] / 2);
        }
    } else {
        memcpy(display_img, dim->img, size);
    }

    // Affichage de l'image
    double img_width = g2x_GetXMax() - g2x_GetXMin();
    double img_height = g2x_GetYMax() - g2x_GetYMin();
    if (show_histogram) img_height *= 0.7; // Réserve de l'espace pour l'histogramme

    double scale_x = img_width / dim->width;
    double scale_y = img_height / dim->height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;

    // Position de départ pour centrer l'image
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

        // Calcul de l'histogramme
        for (int i = 0; i < size; i++) {
            histogram[display_img[i]]++;
            if (histogram[display_img[i]] > max_count) {
                max_count = histogram[display_img[i]];
            }
        }

        // Zone de dessin de l'histogramme
        double hist_y = g2x_GetYMin();
        double hist_height = g2x_GetYMax() * 0.2;
        double hist_width = g2x_GetXMax() - g2x_GetXMin();
        double bar_width = hist_width / 256.0;

        // Dessin des barres
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

    // Affichage du taux de compression
    char info[64];
    snprintf(info, sizeof(info), "Compression: %.2f:1", compression_rate);
    g2x_WriteString(g2x_GetXMin() + 10, g2x_GetYMax() - 20, info);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <input.pgm>\n", argv[0]);
        return 1;
    }

    // Chargement de l'image
    int width, height;
    unsigned char* img = load_pgm(argv[1], &width, &height);
    if (!img) {
        printf("Failed to load image %s\n", argv[1]);
        return 1;
    }

    // Création de l'image différentielle
    dim = create_difimg(width, height);
    if (!dim) {
        printf("Failed to create differential image\n");
        free(img);
        return 1;
    }

    compute_differential(dim, img);

    // Buffer pour l'affichage
    display_img = (unsigned char*)malloc(width * height);
    if (!display_img) {
        printf("Failed to allocate display buffer\n");
        free_difimg(dim);
        free(img);
        return 1;
    }

    // Compression
    uint8_t* buffer;
    size_t bufsize;
    if (!compress_data(dim, &buffer, &bufsize)) {
        printf("Compression failed\n");
        free(display_img);
        free_difimg(dim);
        free(img);
        return 1;
    }

    // Calcul du taux de compression
    compression_rate = (float)(width * height) / (float)bufsize;

    // Sauvegarde du fichier DIFF
    char output_file[256];
    const char* basename = strrchr(argv[1], '/') ? strrchr(argv[1], '/') + 1 : argv[1];
    snprintf(output_file, sizeof(output_file), "./DIFF/%s.dif", basename);

    FILE* f = fopen(output_file, "wb");
    if (!f) {
        printf("Failed to create output file\n");
        free(buffer);
        free(display_img);
        free_difimg(dim);
        free(img);
        return 1;
    }

    // Écriture de l'en-tête
    uint16_t magic = MAGIC_NUMBER;
    uint16_t w = width;
    uint16_t h = height;
    uint8_t num_levels = NUM_LEVELS;
    uint8_t level_bits[4] = {1, 2, 4, 8};

    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&w, sizeof(w), 1, f);
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&num_levels, sizeof(num_levels), 1, f);
    fwrite(level_bits, sizeof(level_bits), 1, f);
    fwrite(&dim->first, sizeof(dim->first), 1, f);
    fwrite(buffer, bufsize, 1, f);

    fclose(f);
    free(buffer);
    free(img);

    // Configuration de l'interface graphique
    g2x_InitWindow("Image Encoder", width, height);
    
    // Ajout des contrôles
    g2x_CreateSwitch("Show Differential", &show_differential, "Toggle differential view");
		g2x_CreateSwitch("Show Histogram", &show_histogram, "Toggle histogram view");
    
    // Configuration des fonctions de callback
    g2x_SetDisplayFunc(display_func);
    
    // Lancement de la boucle principale
    return g2x_MainStart();
}