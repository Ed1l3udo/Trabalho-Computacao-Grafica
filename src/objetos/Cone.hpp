#ifndef CONE_HPP
#define CONE_HPP

#include "Objeto.hpp"  // Inclui a classe base 'Objeto'
#include "Vec4.hpp"    // Inclui a classe Vec4 para operações vetoriais
#include "Color.hpp"   // Inclui a classe Color para representar cores
#include "Luz.hpp"     // Inclui a classe Luz para luzes
#include "Transform.hpp"
#include "Colisao.hpp"

class Cone : public Objeto {
public:
    Vec4 centroBase;  // Posição do centro da base do cone
    Vec4 dir;         // Direção do cone (vetor do vértice para a base)
    double raioBase;  // Raio da base do cone
    double altura;    // Altura do cone

    // Propriedades de iluminação
    Vec4 Ke, Kd, Ka;  // Coeficientes de reflexão (emissão, difusa, ambiente)
    double m;          // Expoente especular (brilho)

    // Construtor
    Cone(Vec4 centroBase = {0, 1, 0, 1}, Vec4 dir = {0, 1, 0, 0}, double raioBase = 1, double altura = 2,
         Vec4 Ke = {1, 1, 1, 0}, Vec4 Kd = {1, 1, 1, 0}, Vec4 Ka = {1, 1, 1, 0}, double m = 1.0)
        : centroBase(centroBase), dir(dir), raioBase(raioBase), altura(altura), Ke(Ke), Kd(Kd), Ka(Ka), m(m) {}

    // Método para verificar a interseção com um raio
    bool intersect(const Vec4& origem, const Vec4& dir, Vec4& intersection, double& t) const override {
        // Equação do cone: (x, y, z) é o ponto de interseção com o raio
        // Calcula a equação quadrática para interseção
        Vec4 topo = centroBase + dir * altura;  // O vértice do cone

        // O valor k (raio/altura) define a "inclinação" do cone
        double k = raioBase / altura;
        double k2 = k * k;

        // Vetor de interseção
        Vec4 w = origem - topo;

        // Componentes da equação quadrática
        double alfa = dir.dot(dir) - (1 + k2) * (dir.dot(dir));
        double beta = (w.dot(dir)) - (1 + k2) * (w.dot(dir));
        double a = dir.dot(dir) - (1 + k2) * alfa * alfa;
        double b = (w.dot(dir) - (1 + k2) * alfa * beta) * 2.0;
        double c = w.dot(w) - (1 + k2) * beta * beta;

        double delta = b * b - 4.0 * a * c;

        if (delta < 0.0)
            return false;  // Não há interseção

        double sqrtD = sqrt(delta);
        double t1 = (-b - sqrtD) / (2 * a);
        double t2 = (-b + sqrtD) / (2 * a);
        t = (t1 > 1e-6) ? t1 : ((t2 > 1e-6) ? t2 : -1.0);
        
        if (t < 0.0) return false;
        
        intersection = origem + dir * t;  // Ponto de interseção
        return true;
    }

    // Método para calcular a cor com base na iluminação
    Color calculaCor(const Vec4& origem, const Vec4& intersection, const Luz& luz, const Luz& luzAmb, const Colisao& tipoDeColisao, bool isInShadow) const override {
        Vec4 n;  // Vetor normal
        switch (tipoDeColisao) {
            case Corpo: {
                // Cálculos para o corpo do cone...
                break;
            }
            case Base: {
                n = -dir;  // Normal da base
                break;
            }
        }

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
        Vec4 I = Ie + Id + Ia;

        // Converte para valores RGB [0..255]
        int R = clamp255(I.x * 255.0);
        int G = clamp255(I.y * 255.0);
        int B = clamp255(I.z * 255.0);

        return Color(R, G, B);  // Retorna a cor calculada
    }
};

#endif // CONE_HPP
