#!/bin/bash

echo "===== Nettoyage ====="
make clean
make

echo ""
echo "===== Test 1 : Version séquentielle (domaine par défaut) ====="
./mandelbrot
display mandelbrot.ras &

echo ""
echo "===== Test 2 : Version parallèle avec 4 threads ====="
export OMP_NUM_THREADS=4
./mandelbrot-parallel
display mandelbrot-parallel.ras &

echo ""
echo "===== Test 3 : Zoom 1 ====="
./mandelbrot 800 800 0.35 0.355 0.353 0.358 200
cp mandelbrot.ras zoom1.ras
display zoom1.ras &

echo ""
echo "===== Test 4 : Zoom 2 ====="
./mandelbrot 800 800 -0.736 -0.184 -0.735 -0.183 500
cp mandelbrot.ras zoom2.ras
display zoom2.ras &

echo ""
echo "===== Test 5 : Zoom 3 ====="
./mandelbrot 800 800 -1.48478 0.00006 -1.48440 0.00044 100
cp mandelbrot.ras zoom3.ras
display zoom3.ras &

echo ""
echo "🎉 Tous les tests sont terminés !"

