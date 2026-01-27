#ifndef PLANO_HPP
#define PLANO_HPP

#include "Objeto.hpp"  
#include "Vec4.hpp"  
#include "Color.hpp"   
#include "Luz.hpp"     
#include "Transform.hpp"

class Plano : public Objeto {
public:
    Vec4 n;        // Vetor normal ao plano
    Vec4 Pi;       // Ponto de referência no plano

    // Propriedades de iluminação
    Vec4 Ke, Kd, Ka;  // Coeficientes de reflexão (emissão, difusa, ambiente)
    double m;          // Expoente especular (brilho)

    // Construtor
    Plano(Vec4 n = {0, 1, 0, 0}, Vec4 Pi = {0, 0, 0, 1}, Vec4 Ke = {0, 0, 0, 0}, Vec4 Kd = {0, 0, 0, 0}, Vec4 Ka = {0, 0, 0, 0}, double m = 1)
        : n(n), Pi(Pi), Ke(Ke), Kd(Kd), Ka(Ka), m(m) {}

    // Método para verificar a interseção com um raio
    bool intersect(const Vec4& origem, const Vec4& rayDir, Vec4& intersection, double& t, Colisao& tipoDeColisao) const override {
        double denom = n.dot(rayDir);  // Verifica se o raio é paralelo ao plano
        if (denom == 0) {
            return false;  // Raio paralelo ao plano, sem interseção
        }

        // Calcula a interseção do raio com o plano
        Vec4 w = origem - Pi;
        t = -(n.dot(w)) / denom;
        if (t < 0) {
            return false;  // Interseção atrás da origem do raio
        }

        // Calcula o ponto de interseção
        intersection = origem + rayDir * t;
        return true;
    }

    // Método para calcular a cor com base na iluminação
    Color calculaCor(const Vec4& origem, const Vec4& intersection, const Luz& luz, const Luz& luzAmb, const Colisao& tipoDeColisao, bool isInShadow) const override {
        Vec4 l = normalize(luz.pos - intersection);  // Vetor para a luz
        Vec4 v = normalize(origem - intersection);   // Vetor para o observador
        double cosNL = std::max(0.0, n.dot(l));      // Cálculo do ângulo de incidência (difuso)

        // Vetor de reflexão
        Vec4 r = normalize(n * (2 * cosNL) - l);
        double cosVR = std::max(0.0, v.dot(r));  // Cálculo da reflexão para especular

        // Potência especular
        double specpow = (cosVR > 0.0) ? std::pow(cosVR, m) : 0.0;

        // Componentes de luz
        Vec4 Ie = hadamard(luz.intensidade, Ke) * specpow;  // Emissão
        Vec4 Id = hadamard(luz.intensidade, Kd) * cosNL;    // Difusa
        Vec4 Ia = hadamard(luzAmb.intensidade, Ka);          // Ambiente
        Vec4 I = Ia;
        if(!isInShadow){
            I = Ie + Id + Ia;
        }

        // Converte para valores RGB [0..255]
        int R = clamp255(I.x * 255.0);
        int G = clamp255(I.y * 255.0);
        int B = clamp255(I.z * 255.0);

        return Color(R, G, B);  // Retorna a cor calculada
    }
};

#endif // PLANO_HPP
