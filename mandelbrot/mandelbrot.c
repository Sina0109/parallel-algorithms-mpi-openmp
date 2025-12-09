/**
 * Calcul de l'ensemble de Mandelbrot
 *
 * Version originale par Dominique Béréziat, LIP6, Sorbonne Université, 2004.
 *
 * Adapté pour le cours de Programmation parallèle et distribuée par Marek
 * Felšöci, LI-PaRAD, Université de Versailles - Saint Quentin en Yvelines,
 * 2025.
 */

#include <stdlib.h>
#include <stdio.h>
// Ce programme utilise la fonction `omp_get_wtime()' d'OpenMP pour le
// chronométrage du calcul.
#include <omp.h>
// Le fichier suivant défini des éléments du format d'image Raster utilisé pour
// sauvegarder le résultat du calcul.
#include "rasterfile.h"

char usage[] =
  "Usage: %s DIMX DIMY XMIN YMIN XMAX YMAX PROF\n"
  "Calculer l'ensemble de Mandelbrot étant donné les paramètres ci-dessous.\n\n"
  "Paramètres :\n"
  " - DIMX, DIMY : dimensions de l'image à générer\n"
  " - XMIN, YMIN, XMAX, YMAX, : domaine à calculer dans le plan complexe\n"
  " - PROF : nombre maximal d'itérations\n\n"
  "Quelques exemples d'exécution :\n"
  " - %s %d %d %lg %lg %lg %lg %d (paramétrage par défaut)\n"
  " - %s 800 800 0.35 0.355 0.353 0.358 200\n"
  " - %s 800 800 -0.736 -0.184 -0.735 -0.183 500\n"
  " - %s 800 800 -0.736 -0.184 -0.735 -0.183 300\n"
  " - %s 800 800 -1.48478 0.00006 -1.48440 0.00044 100\n"
  " - %s 800 800 -1.5 -0.1 -1.3 0.1 10000\n\n";

/**
 * Convertir un entier Linux (4 octets) en un entier Sun.
 * @param i Entier à convertir.
 * @return Entier converti.
 */
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

/**
 *  Sauvegarder l'image finale au format Raster (8 bits avec une palette de 256
 *  niveaux de gris du blanc, c'est-à-dire 0, vers le noir, c'est-à-dire 255).
 *  @param nom Nom du fichier cible.
 *  @param largeur Largeur de l'image.
 *  @param hauteur Hauteur de l'image.
 *  @param p Pointeur vers la zone mémoire contenant l'image à sauvegarder.
 */
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
  // profondeur de chaque pixel (1, 8 ou 24)
  ras.ras_depth = swap(8);
  ras.ras_length = swap(largeur * hauteur);
  ras.ras_type = swap(RT_STANDARD);
  ras.ras_maptype = swap(RMT_EQUAL_RGB);
  ras.ras_maplength = swap(256 * 3);

  fwrite(&ras, sizeof(struct rasterfile), 1, fd); 
  
  // palette de couleurs (composante rouge)
  i = 256;
  while(i--) {
    o= i / 2;
    fwrite(&o, sizeof(unsigned char), 1, fd);
  }

  // palette de couleurs (composante verte)
  i = 256;
  while(i--) {
    o = (i % 190);
    fwrite(&o, sizeof(unsigned char), 1, fd);
  }

  // palette de couleurs (composante bleu)
  i = 256;
  while(i--) {
    unsigned char o = (i % 120) * 2;
    fwrite(&o, sizeof(unsigned char), 1, fd);
  }

  fwrite(p, largeur * hauteur, sizeof(unsigned char), fd);
  fclose(fd);
}

/**
 * Étant donnée les coordonnées d'un point \f$ c = a + ib \f$ dans le plan
 * complexe, retourner la couleur correspondante estimant à quelle distance de
 * l'ensemble de Mandelbrot le point se trouve.
 *
 * Soit la suite complexe défini par :
 * \f[
 * \left \{ \begin{array}{l}
 * z_0 = 0 \\
 * z_{n+1} = z_n^2 - c
 * \end{array} \right.
 * \f]
 *
 * Le nombre d'itérations que la suite met pour diverger est le nombre \f$ n \f$
 * pour lequel \f$ |z_n| > 2 \f$. Ce nombre est ramené à une valeur entre 0 et
 * 255 correspond ainsi à une couleur dans la palette des couleurs.
 */
unsigned char xy2color(double a, double b, int prof) {
  double x, y, temp, x2, y2;
  int i;

  x = y = 0.;
  for(i = 0; i < prof; i++) {
    // Garder la valeur précédente de `x' qui va être écrasée.
    temp = x;
    // Calculer les nouvelles valeurs de `x' et `y'.
    x2 = x * x;
    y2 = y * y;
    x = x2 - y2 + a;
    y = 2 * temp * y + b;
    if(x2 + y2 >= 4.0) break;
  }

  return (i == prof) ? 255: (int) ((i % 255)); 
}

/** 
 * Fonction principale : en chaque point de la grille, appliquer `xy2color'.
 */
int main(int argc, char ** argv) {
  // Domaine de calcul dans le plan complexe.
  double xmin, ymin;
  double xmax, ymax;
  // Dimensions de l'image.
  int w, h;
  // Pas d'incrémentation.
  double xinc, yinc;
  // Profondeur d'itération.
  int prof;
  // Image finale.
  unsigned char	* grid, * pgrid;
  // Variables intermédiaires.
  int i, j;
  double x, y;
  // Chronométrage.
  double debut, fin;
  
  // Paramètres par défaut de la fractale.
  xmin = -2; ymin = -2;
  xmax =  2; ymax =  2;
  w = h = 800;
  prof = 200;

  // Affichage du message d'accueil.
  if(argc == 1) {
    fprintf(
      stderr, usage, argv[0], argv[0], w, h, xmin, ymin, xmax, ymax, prof,
      argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]
    );
  }
  
  // Récupération des paramètres à partir de la ligne de commande.
  if(argc > 1) w    = atoi(argv[1]);
  if(argc > 2) h    = atoi(argv[2]);
  if(argc > 3) xmin = atof(argv[3]);
  if(argc > 4) ymin = atof(argv[4]);
  if(argc > 5) xmax = atof(argv[5]);
  if(argc > 6) ymax = atof(argv[6]);
  if(argc > 7) prof = atoi(argv[7]);

  // Calcul des pas d'incrémentation.
  xinc = (xmax - xmin) / (w - 1);
  yinc = (ymax - ymin) / (h - 1);
  
  // Affichage des paramètres pour vérification.
  fprintf(
    stderr, "Domaine: {[%lg, %lg] x [%lg, %lg]}\n", xmin, ymin, xmax, ymax
  );
  fprintf(stderr, "Incrément : %lg %lg\n", xinc, yinc);
  fprintf(stderr, "Profondeur : %d\n",  prof);
  fprintf(stderr, "Dimensions de l'image: %d x %d\n\n", w, h);
  
  // Allocation de la mémoire pour la grille représentant l'image finale.  
  pgrid = grid = (unsigned char *) malloc(w * h * sizeof(unsigned char));
  if(grid == NULL) {
    fprintf(
      stderr, "Erreur à l'allocation mémoire de la grille de l'image !\n"
    );
    return 1;
  }

  // Début du chronométrage.
  debut = omp_get_wtime();
  
  // Traitement de la grille point par point (calcul principal).
  for(y = ymin, i = 0; i < h; y += yinc, i++) {	
    for(x = xmin, j = 0; j < w; x += xinc, j++) {
      *pgrid++ = xy2color(x, y, prof); 
    }
  }

  // Fin du chronométrage.
  fin = omp_get_wtime();
  
  // Sauvegarde de la grille de l'image dans le fichier `mandelbrot.ras'.
  sauver_rasterfile("mandelbrot.ras", w, h, grid);

  // Affichage du temps de calcul mesuré en secondes.
  fprintf(stderr, "Temps total de calcul : %g s\n", fin - debut);
  return 0;
}
