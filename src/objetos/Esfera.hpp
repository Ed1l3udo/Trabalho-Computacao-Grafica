#ifndef ESFERA_HPP
#define ESFERA_HPP

#include "Objeto.hpp"  // Inclui a classe base 'Objeto'
#include "Color.hpp"   // Inclui a classe Color
#include "Luz.hpp"     // Inclui a classe Luz
#include "Vec4.hpp"    // Inclui a classe Vec4
#include "Transform.hpp" // Inclui a classe Transform

class Esfera : public Objeto {
public:
    Vec4 centro;  // Posição do centro da esfera
    double raio;  // Raio da esfera

    // Propriedades de iluminação
    Vec4 Ke, Kd, Ka;  // Coeficientes de reflexão (emissão, difusa, ambiente)
    double m;          // Expoente especular (brilho)

    // Construtor
    Esfera(Vec4 centro = {0,0,0,1}, double raio = 1, Vec4 Ke = {0, 0, 0, 0}, Vec4 Kd = {0, 0, 0, 0}, Vec4 Ka = {0, 0, 0, 0}, double m = 1.0)
        : centro(centro), raio(raio), Ke(Ke), Kd(Kd), Ka(Ka), m(m) {}

    // Método para verificar a interseção com um raio
    bool intersect(const Vec4& origem, const Vec4& dir, Vec4& intersection, double& t) const override {
        Vec4 w = origem - centro;
        double B = 2.0 * w.dot(dir);
        double C = w.dot(w) - raio * raio;
        double delta = B * B - 4.0 * C;
        
        if (delta < 0.0) return false;  // Não há interseção

        double sqrtD = std::sqrt(delta);
        double t1 = (-B - sqrtD) / 2.0;  // Primeira solução
        double t2 = (-B + sqrtD) / 2.0;  // Segunda solução

        // Seleciona o menor t que seja positivo
        t = (t1 > 1e-6) ? t1 : (t2 > 1e-6) ? t2 : -1.0;
        if (t < 0.0) return false;

        // Calcula o ponto de interseção
        intersection = origem + dir * t;
        return true;
    }

    // Método para calcular a cor com base na iluminação
    Color calculaCor(const Vec4& origem, const Vec4& intersection, const Luz& luz, const Luz& luzAmb, bool isInShadow) const override {
        Vec4 n = normalize(intersection - centro);  // Vetor normal da superfície
        Vec4 l = normalize(luz.pos - intersection);  // Vetor para a luz
        Vec4 v = normalize(origem - intersection);   // Vetor para o observador
        double cosNL = std::max(0.0, n.dot(l));  // Cálculo do ângulo de incidência

        // Vetor de reflexão
        Vec4 r = normalize(n * (2 * cosNL) - l);
        double cosVR = std::max(0.0, v.dot(r));  // Cálculo da reflexão para especular

        // Potência especular
        double specpow = (cosVR > 0.0) ? std::pow(cosVR, m) : 0.0;

        // Componentes de luz
        Vec4 Ie = hadamard(luz.intensidade, Ke) * specpow;  // Emissão
        Vec4 Id = hadamard(luz.intensidade, Kd) * cosNL;    // Difusa
        Vec4 Ia = hadamard(luzAmb.intensidade, Ka);          // Ambiente
        Vec4 I = Ie + Id + Ia;

        // Converte para valores RGB [0..255]
        int R = clamp255(I.x * 255.0);
        int G = clamp255(I.y * 255.0);
        int B = clamp255(I.z * 255.0);

        return Color(R, G, B);  // Retorna a cor calculada
    }
};

#endif // ESFERA_HPP
