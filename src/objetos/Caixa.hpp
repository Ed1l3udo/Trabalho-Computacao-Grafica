#pragma once
#include <cmath>
#include <algorithm>

#include "Objeto.hpp"
#include "Vec4.hpp"
#include "Color.hpp"
#include "Luz.hpp"
#include "Colisao.hpp"
#include "Transform.hpp" // hadamard, normalize, etc

class Caixa : public Objeto {
public:
    // Caixa alinhada aos eixos no espaço LOCAL: min <= (x,y,z) <= max
    Vec4 bmin; // w=1
    Vec4 bmax; // w=1

    // “Materiais” no seu padrão
    Vec4 Ke, Kd, Ka;   // Ke = especular (seu “Ks”), Kd = difuso, Ka = ambiente
    double m;          // brilho

    // normal do último hit no espaço local (para shading)
    mutable Vec4 lastNormalLocal = Vec4(0, 1, 0, 0);

    Caixa(
        Vec4 bmin = Vec4(0,0,0,1),
        Vec4 bmax = Vec4(1,1,1,1),
        Vec4 Ke = Vec4(0.5,0.5,0.5,0),
        Vec4 Kd = Vec4(0.8,0.8,0.8,0),
        Vec4 Ka = Vec4(0.1,0.1,0.1,0),
        double m = 50.0
    ) : bmin(bmin), bmax(bmax), Ke(Ke), Kd(Kd), Ka(Ka), m(m) {}

    bool intersectLocal(const Vec4& origemL, const Vec4& dirL,
                        Vec4& intersectionL, double& tL, Colisao& tipo) const override
    {
        const double EPS = 1e-9;

        double tmin = -1e18;
        double tmax =  1e18;

        Vec4 nEnter(0,0,0,0);
        Vec4 nExit (0,0,0,0);

        auto slab = [&](double o, double d, double mn, double mx,
                        Vec4 nMin, Vec4 nMax) -> bool
        {
            if (std::abs(d) < EPS) {
                // Raio paralelo às faces desse eixo
                if (o < mn || o > mx) return false;
                return true;
            }

            double t1 = (mn - o) / d;
            double t2 = (mx - o) / d;

            Vec4 n1 = nMin;
            Vec4 n2 = nMax;

            // garante t1 <= t2
            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(n1, n2);
            }

            // Atualiza intervalo
            if (t1 > tmin) { tmin = t1; nEnter = n1; }
            if (t2 < tmax) { tmax = t2; nExit  = n2; }

            if (tmax < tmin) return false;
            return true;
        };

        // Para cada eixo, definimos normal da face MIN e MAX (no espaço local)
        // Entrando por x=bmin -> normal (-1,0,0), por x=bmax -> normal (+1,0,0)
        if (!slab(origemL.x, dirL.x, bmin.x, bmax.x, Vec4(-1,0,0,0), Vec4( 1,0,0,0))) return false;
        if (!slab(origemL.y, dirL.y, bmin.y, bmax.y, Vec4(0,-1,0,0), Vec4(0, 1,0,0))) return false;
        if (!slab(origemL.z, dirL.z, bmin.z, bmax.z, Vec4(0,0,-1,0), Vec4(0,0, 1,0))) return false;

        // Se tmax < 0, a caixa está toda atrás do raio
        if (tmax <= EPS) return false;

        // Se o raio começa fora, usamos tmin. Se começa dentro, tmin < 0 então usamos tmax.
        if (tmin > EPS) {
            tL = tmin;
            lastNormalLocal = nEnter;
        } else {
            tL = tmax;
            lastNormalLocal = nExit;
        }

        intersectionL = origemL + dirL * tL;
        tipo = Colisao::Corpo; // caixa não tem Base/Topo aqui
        return true;
    }

    Color calculaCor(const Vec4& origemW, const Vec4& intersectionW,
                     const Luz& luz, const Luz& luzAmb,
                     const Colisao& /*tipo*/, bool isInShadow) const override
    {
        // normal no mundo via normal matrix
        Vec4 n = normalToWorld(lastNormalLocal);

        Vec4 Ia = hadamard(luzAmb.intensidade, Ka);

        Vec4 I = Ia;
        if (!isInShadow) {
            Vec4 l = normalize(luz.pos - intersectionW);
            Vec4 v = normalize(origemW - intersectionW);

            double cosNL = std::max(0.0, n.dot(l));

            Vec4 r = normalize(n * (2.0 * cosNL) - l);
            double cosVR = std::max(0.0, v.dot(r));
            double specpow = (cosVR > 0.0) ? std::pow(cosVR, m) : 0.0;

            Vec4 Id = hadamard(luz.intensidade, Kd) * cosNL;
            Vec4 Is = hadamard(luz.intensidade, Ke) * specpow; // Ke = especular (seu “Ks”)

            I = Ia + Id + Is;
        }

        auto sat01 = [](double x){ return std::max(0.0, std::min(1.0, x)); };
        int R = (int)(sat01(I.x) * 255.0);
        int G = (int)(sat01(I.y) * 255.0);
        int B = (int)(sat01(I.z) * 255.0);

        return Color(R,G,B);
    }
};
