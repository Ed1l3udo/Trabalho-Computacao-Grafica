#ifndef LUZ_HPP
#define LUZ_HPP

#include <cmath>
#include "Vec4.hpp"
#include "Transform.hpp"  // normalize()

enum class TipoLuz { Ambiente, Pontual, Direcional, Spot };

class Luz {
public:
    TipoLuz tipo;
    Vec4 pos;          // usado em Pontual/Spot (w=1)
    Vec4 dir;          // usado em Direcional/Spot (w=0), direção que a luz "aponta"
    Vec4 intensidade;  // RGB em [0..1], w=0

    // Spot: cutoff em cos(angulo). Ex: 20 graus -> cos(20°)
    double cutoffCos = -1.0;

    static constexpr double kPi = 3.14159265358979323846;

    // Pontual
    Luz(const Vec4& pos, const Vec4& intensidade)
        : tipo(TipoLuz::Pontual), pos(pos), dir(0, -1, 0, 0), intensidade(intensidade) {}

    // Ambiente
    Luz(const Vec4& intensidade)
        : tipo(TipoLuz::Ambiente), pos(0, 0, 0, 1), dir(0, -1, 0, 0), intensidade(intensidade) {}

    // Direcional: dir é a direção que os raios viajam (do "sol" para a cena)
    static Luz Direcional(const Vec4& dir, const Vec4& intensidade) {
        Luz L(intensidade);
        L.tipo = TipoLuz::Direcional;
        L.dir = normalize(Vec4(dir.x, dir.y, dir.z, 0));
        return L;
    }

    // Spot: pos + direção + cutoff em graus
    static Luz Spot(const Vec4& pos, const Vec4& dir, double cutoffDeg, const Vec4& intensidade) {
        Luz L(pos, intensidade);
        L.tipo = TipoLuz::Spot;
        L.dir = normalize(Vec4(dir.x, dir.y, dir.z, 0));
        L.cutoffCos = std::cos(cutoffDeg * kPi / 180.0);
        return L;
    }

    // Vetor unitário do ponto P até a luz (direção de iluminação)
    Vec4 Lvec(const Vec4& P) const {
        if (tipo == TipoLuz::Direcional) {
            // direcional: "luz vem de -dir"
            return normalize(-dir);
        }
        // pontual/spot: luz vem de (pos - P)
        return normalize(pos - P);
    }

    // Distância máxima para sombra (pontual/spot tem limite; direcional é "infinito")
    double maxDistance(const Vec4& P) const {
        if (tipo == TipoLuz::Direcional) return 1e30;
        return (pos - P).length();
    }

    // Spot: retorna 0 se o ponto estiver fora do cone; 1 se estiver dentro.
    double factorAt(const Vec4& P) const {
        if (tipo != TipoLuz::Spot) return 1.0;

        // vetor da luz para o ponto
        Vec4 s = normalize(P - pos);
        double cosTheta = dir.dot(s); // dir e s unitários

        return (cosTheta >= cutoffCos) ? 1.0 : 0.0;
    }
};

#endif // LUZ_HPP
