#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "rasterfile.h"

/* Convertir endianness */
int swap(int i) {
  int init = i; 
  int conv;
  unsigned char * o, * d;

  o = ((unsigned char *) &init) + 3; 
  d = (unsigned char *) &conv;

  *d++ = *o--;
  *d++ = *o--;
  *d++ = *o--;
  *d++ = *o--;

  return conv;
}

/* Sauvegarde d'image */
void sauver_rasterfile(char * nom, int largeur, int hauteur, unsigned char * p) {
  FILE *fd;
  struct rasterfile ras;
  int i;
  unsigned char o;

  if((fd = fopen(nom, "w")) == NULL) {
    printf("Erreur à la création du fichier %s !\n", nom);
    exit(EXIT_FAILURE);
  }

  ras.ras_magic = swap(RAS_MAGIC);	
  ras.ras_width = swap(largeur);
  ras.ras_height = swap(hauteur);
  ras.ras_depth = swap(8);
  ras.ras_length = swap(largeur * hauteur);
  ras.ras_type = swap(RT_STANDARD);
  ras.ras_maptype = swap(RMT_EQUAL_RGB);
  ras.ras_maplength = swap(256 * 3);

  fwrite(&ras, sizeof(struct rasterfile), 1, fd); 
  
  for(i = 0; i < 256; i++) { o = i/2; fwrite(&o, 1, 1, fd); }
  for(i = 0; i < 256; i++) { o = i%190; fwrite(&o, 1, 1, fd); }
  for(i = 0; i < 256; i++) { o = (i%120)*2; fwrite(&o, 1, 1, fd); }

  fwrite(p, largeur * hauteur, 1, fd);
  fclose(fd);
}

/* Calcul couleur */
unsigned char xy2color(double a, double b, int prof) {
  double x, y, temp, x2, y2;
  int i;

  x = y = 0.;
  for(i = 0; i < prof; i++) {
    temp = x;
    x2 = x*x;
    y2 = y*y;
    x = x2 - y2 + a;
    y = 2 * temp * y + b;
    if(x2 + y2 >= 4.0) break;
  }
  return (i == prof) ? 255 : (i % 255);
}

/* Version PARALLÈLE */
int main(int argc, char ** argv) {
  double xmin=-2, ymin=-2, xmax=2, ymax=2;
  int w=800, h=800, prof=200;

  if(argc > 1) w    = atoi(argv[1]);
  if(argc > 2) h    = atoi(argv[2]);
  if(argc > 3) xmin = atof(argv[3]);
  if(argc > 4) ymin = atof(argv[4]);
  if(argc > 5) xmax = atof(argv[5]);
  if(argc > 6) ymax = atof(argv[6]);
  if(argc > 7) prof = atoi(argv[7]);

  double xinc = (xmax - xmin) / (w - 1);
  double yinc = (ymax - ymin) / (h - 1);

  unsigned char *grid = malloc(w * h);
  if(!grid) return 1;

  double debut = omp_get_wtime();

  /* BOUCLE PARALLÈLE */
#pragma  omp parallel for collapse(2) schedule(dynamic)
for(int i= 0; i<h; i++){
	for(int j = 0; j < w; j++){
	double y = ymin + i * yinc;
	double x = xmin + j * xinc;
	grid[i*w + j] = xy2color(x, y, prof);
}
}  
  double fin = omp_get_wtime();

  sauver_rasterfile("mandelbrot-parallel.ras", w, h, grid);

  fprintf(stderr, "Temps total (parallèle) : %g s\n", fin - debut);

  free(grid);
  return 0;
}

