CC = gcc
CFLAGS = -Wall -Wextra -I./include -I/usr/include/g2x
LDFLAGS = -L/usr/lib/g2x -Wl,-rpath=/usr/lib/g2x -lg2x -lglut -lGLU -lGL -lm

# Dossiers
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Objets communs (excluant les fichiers main)
COMMON_SOURCES = $(SRCDIR)/codec.c $(SRCDIR)/imgdif.c
COMMON_OBJECTS = $(COMMON_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Executables
ENCODER = pgmtodif
DECODER = diftopgm

all: directories $(ENCODER) $(DECODER)

directories:
	mkdir -p $(OBJDIR) PGM DIFF

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ENCODER): $(COMMON_OBJECTS) $(OBJDIR)/pgmtodif.o
	$(CC) $^ -o $@ $(LDFLAGS)

$(DECODER): $(COMMON_OBJECTS) $(OBJDIR)/diftopgm.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(ENCODER) $(DECODER)

mrproper: clean
	rm -rf PGM/* DIFF/*

.PHONY: all clean mrproper directories