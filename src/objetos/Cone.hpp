#ifndef CONE_HPP
#define CONE_HPP

#include <algorithm>
#include <cmath>

#include "Objeto.hpp"  // Inclui a classe base 'Objeto'
#include "Vec4.hpp"    // Inclui a classe Vec4 para operações vetoriais
#include "Color.hpp"   // Inclui a classe Color para representar cores
#include "Luz.hpp"     // Inclui a classe Luz para luzes
#include "Transform.hpp"
#include "Colisao.hpp"
#include "RayPlane.hpp"


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
        : centroBase(centroBase), dir(normalize(dir)), raioBase(raioBase), altura(altura), Ke(Ke), Kd(Kd), Ka(Ka), m(m) {}

    // Método para verificar a interseção com um raio
    bool intersectLocal(const Vec4& origin, const Vec4& dir, Vec4& intersection, double& t, Colisao& tipoDeColisao) const override
    {
        // Corpo
        const double EPS = 1e-6;
        bool encostou_corpo = false;
        Vec4 intersection_corpo;

        Vec4 topo = this->centroBase + this->dir * this->altura;

        double t_corpo = -1.0;
        if (std::abs(this->altura) > EPS) {
            double k = this->raioBase/this->altura;
            double k2 = k*k;
            Vec4 w = origin - topo;
            double alfa = dir.dot(this->dir);
            double beta = w.dot(this->dir);

            double a = dir.dot(dir) - (1 + k2) * alfa * alfa;
            double b = (w.dot(dir) - (1 + k2) * alfa * beta) * 2.0;
            double c = w.dot(w) - (1 + k2) * beta * beta;
            double delta = b*b - 4.0*a*c;

            if (std::abs(a) > EPS) {
                if (delta >= -EPS) {
                    double sqrtD = std::sqrt(std::max(0.0, delta));
                    double t1 = (-b - sqrtD)/(2.0*a);
                    double t2 = (-b + sqrtD)/(2.0*a);

                    if (std::isfinite(t1) && t1 > EPS) t_corpo = t1;
                    if (std::isfinite(t2) && t2 > EPS && (t_corpo < 0.0 || t2 < t_corpo)) t_corpo = t2;
                }
            } else if (std::abs(b) > EPS) {
                double t_linear = -c / b;
                if (std::isfinite(t_linear) && t_linear > EPS) t_corpo = t_linear;
            }
        }

        if(t_corpo < 0.0)
        {
            encostou_corpo = false;
        }
        else
        {
            double z = ((origin + dir * t_corpo) - this->centroBase).dot(this->dir);
            if(0.0 <= z && z <= this->altura)
            {
                encostou_corpo = true;
            }
            else
            {
                encostou_corpo = false;
            }
        }

        if(encostou_corpo)
        {
            intersection_corpo = origin + dir * t_corpo;
        }

        // Base
        bool encostou_base = false;
        Vec4 intersection_planoBase;
        double t_base = 0.0;
        bool encostou_planoBase = intersect_ray_plane(origin, dir, this->centroBase, this->dir, intersection_planoBase, t_base);

        if(!encostou_planoBase)
        {
            encostou_base = false;
        }
        else
        {
            double dist = (intersection_planoBase - this->centroBase).length();
            if(dist <= this->raioBase)
            {
                encostou_base = true;
            }
            else
            {
                encostou_base = false;
            }
        }

        // Verificando qual mostrar
        if (!encostou_corpo && !encostou_base) return false;

        if (encostou_corpo && (!encostou_base || t_corpo < t_base)) {
            intersection = intersection_corpo;
            t = t_corpo;
            tipoDeColisao = Corpo;
        } else {
            intersection = intersection_planoBase;
            t = t_base;
            tipoDeColisao = Base;
        }
        return true;
    }

    // Método para calcular a cor com base na iluminação
    Color calculaCor(const Vec4& origem, const Vec4& intersection, const Luz& luz, const Luz& luzAmb, const Colisao& tipoDeColisao, bool isInShadow) const override {
    
        Vec4 pL = toLocalPoint(intersection);
        Vec4 nL(0,0,0,0);
        switch (tipoDeColisao) {
            case Corpo: {
                Vec4 topo = this->centroBase + this->dir * this->altura;
                double k = this->raioBase/this->altura;

                Vec4 topToIntersection = pL - topo;
                Vec4 topToIntersection_Y = this->dir * (topToIntersection.dot(this->dir));
                Vec4 topToIntersection_X = topToIntersection - topToIntersection_Y;
                nL = normalize(topToIntersection_X - topToIntersection_Y*(k*k));
                break;
            }
            case Base: {
                nL = -(this->dir);
                break;
            }
            case Topo:
                // Cone não possui tampa superior; este valor não é produzido por intersectLocal.
                break;
            case Nenhuma:
                // calculaCor só deve ser chamado após um hit; mantém o resultado finito se o contrato for violado.
                break;
        }
        Vec4 n = normalToWorld(nL);

        double fatt = luz.factorAt(intersection);
        Vec4 I_luz = luz.intensidade * fatt;
        Vec4 l = luz.Lvec(intersection);

        Vec4 v = normalize(origem - intersection);   // Vetor para o observador
        double cosNL = std::max(0.0, n.dot(l));      // Cálculo do ângulo de incidência (difuso)

        // Vetor de reflexão
        Vec4 r = normalize(n * (2 * cosNL) - l);
        double cosVR = std::max(0.0, v.dot(r));  // Cálculo da reflexão para especular

        // Potência especular
        double specpow = (cosVR > 0.0) ? std::pow(cosVR, m) : 0.0;

        // Componentes de luz
        Vec4 Id = hadamard(I_luz, Kd) * cosNL;
        Vec4 Ie = hadamard(I_luz, Ke) * specpow;
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

#endif // CONE_HPP
