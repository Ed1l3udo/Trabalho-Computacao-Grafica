#pragma once
#include "Vec4.hpp"
#include "Transform.hpp"
#include <cmath>

constexpr double PI_D = 3.14159265358979323846;
enum class Projecao { Perspectiva, Ortografica, Obliqua };

struct Camera {
    Vec4 eye;   // w=1
    Vec4 at;    // w=1
    Vec4 up;    // w=0

    double d;          // distância focal
    double xmin, xmax; // janela na câmera
    double ymin, ymax;

    Projecao proj = Projecao::Perspectiva;

    // só para oblíqua:
    double obliqAlpha = 45.0 * PI_D / 180.0; // direção em X
    double obliqPhi   = 45.0 * PI_D / 180.0; // direção em Y
    double obliqL      = 1.0;                // “força” da inclinação

    // gera o raio que sai do eye e atravessa o pixel (i,j)
    void geraRaio(int i, int j, int width, int height, Vec4& origem, Vec4& dir) const {
        // Base da câmera (u,v,w)
        Vec4 wcam = normalize(eye - at);           // para trás
        Vec4 ucam = normalize(up.cross(wcam));     // direita
        Vec4 vcam = wcam.cross(ucam);              // cima

        // coordenadas (x,y) na janela
        double u = xmin + (xmax - xmin) * ((i + 0.5) / (double)width);
        double v = ymax - (ymax - ymin) * ((j + 0.5) / (double)height);

        // ponto na janela (plano a distância d)
        Vec4 p = eye + (ucam * u) + (vcam * v) - (wcam * d);

        origem = eye;
        dir = normalize(p - eye);
    }
};