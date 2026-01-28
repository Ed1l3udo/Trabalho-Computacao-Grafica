#pragma once
#include "Vec4.hpp"
#include "Transform.hpp"
#include <cmath>

enum class Projecao { Perspectiva, Ortografica, Obliqua };

struct Camera {
    Vec4 eye;   // w=1
    Vec4 at;    // w=1
    Vec4 up;    // w=0

    double d;          // distância focal
    double xmin, xmax; // janela da câmera
    double ymin, ymax;

    Projecao proj = Projecao::Perspectiva;

    // parâmetros de oblíqua (Cabinet/Cavalier)
    // direção em coordenadas da câmera
    // dCam = (L*cos(alpha), L*sin(phi), -1)
    double obliqAlpha = 45.0; // graus
    double obliqPhi   = 45.0; // graus
    double obliqL     = 1.0;  // 1.0=cavalier, 0.5=cabinet

    static constexpr double kPi = 3.14159265358979323846;

    void geraRaio(int i, int j, int width, int height, Vec4& origem, Vec4& dir) const {
        // base ortonormal da câmera em mundo
        Vec4 wcam = normalize(eye - at);          // para trás
        Vec4 ucam = normalize(up.cross(wcam));   // direita
        Vec4 vcam = wcam.cross(ucam);            // cima

        // coordenadas (u,v) na janela
        double uu = xmin + (xmax - xmin) * ((i + 0.5) / (double)width);
        double vv = ymax - (ymax - ymin) * ((j + 0.5) / (double)height);

        if (proj == Projecao::Perspectiva) {
            // ponto na janela a distância d
            Vec4 p = eye + (ucam * uu) + (vcam * vv) - (wcam * d);
            origem = eye;
            dir = normalize(p - eye);
            return;
        }

        // Ortográfica / Oblíqua: origem varia por pixel na janela
        origem = eye + (ucam * uu) + (vcam * vv);

        if (proj == Projecao::Ortografica) {
            // direção constante para frente
            dir = normalize(-wcam);
            return;
        }

        // Oblíqua: direção constante inclinada (definida na câmera)
        double a = obliqAlpha * kPi / 180.0;
        double p = obliqPhi   * kPi / 180.0;

        Vec4 dCam(
            obliqL * std::cos(a),
            obliqL * std::sin(p),
            -1.0,
            0
        );

        // converte da base da câmera para mundo
        Vec4 dWorld = (ucam * dCam.x) + (vcam * dCam.y) + ((-wcam) * (-dCam.z));
        dir = normalize(dWorld);
    }
};
