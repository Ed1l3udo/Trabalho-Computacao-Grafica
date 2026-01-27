#ifndef CILINDRO_HPP
#define CILINDRO_HPP

#include "Objeto.hpp"  // Inclui a classe base 'Objeto'
#include "Vec4.hpp"    // Inclui a classe Vec4 para operações vetoriais
#include "Color.hpp"   // Inclui a classe Color para representar cores
#include "Luz.hpp"     // Inclui a classe Luz para luzes
#include "Transform.hpp"
#include "Colisao.hpp" // Inclui a enumeração Colisao
#include "RayPlane.hpp"

class Cilindro : public Objeto {
public:
    Vec4 centroBase;  // Posição do centro da base do cilindro
    Vec4 dir;         // Direção do cilindro (vetor do centro da base para o topo)
    double raioBase;  // Raio da base do cilindro
    double altura;    // Altura do cilindro

    // Propriedades de iluminação
    Vec4 Ke, Kd, Ka;  // Coeficientes de reflexão (emissão, difusa, ambiente)
    double m;          // Expoente especular (brilho)

    // Construtor
    Cilindro(Vec4 centroBase = {0, 0, 0, 1}, Vec4 dir = {0, 1, 0, 0}, double raioBase = 1, double altura = 2, Vec4 Ke = {1, 1, 1, 0}, Vec4 Kd = {1, 1, 1, 0}, Vec4 Ka = {1, 1, 1, 0}, double m = 1.0)
        : centroBase(centroBase), dir(normalize(dir)), raioBase(raioBase), altura(altura), Ke(Ke), Kd(Kd), Ka(Ka), m(m) {}

    // Método para verificar a interseção com um raio
    bool intersect(const Vec4& origem, const Vec4& dir, Vec4& intersection, double& t, Colisao& tipoDeColisao) const override {
{
    // Corpo
    bool encostou_corpo;
    Vec4 intersection_corpo;
    Vec4 w = origem - this->centroBase;
    double alfa = dir.dot(this->dir);
    double beta = w.dot(this->dir);

    double a = dir.dot(dir) - alfa*alfa;
    double b = (dir.dot(w) - alfa*beta) * 2;
    double c = w.dot(w) - beta*beta - this->raioBase*this->raioBase;
    double delta = b*b - 4*a*c;

    if(delta < 0.0)
    {
        encostou_corpo = false;
    }

    double sqrtD = sqrt(delta);
    double t1 = (-b - sqrtD)/(2*a);
    double t2 = (-b + sqrtD)/(2*a);
    double t_corpo = (t1 > 1e-6) ? t1 : ((t2 > 1e-6) ? t2 : -1.0);

    if(t_corpo < 0.0)
    {
        encostou_corpo = false;
    }
    else
    {
        double z = ((origem + dir*t_corpo) - this->centroBase).dot(this->dir); // projeção no eixo do cilindro
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
        intersection_corpo = origem + dir*t_corpo;
    }

    // Base
    bool encostou_base;
    Vec4 intersection_planoBase;
    double t_base;
    bool encostou_planoBase = intersect_ray_plane(origem, dir, this->centroBase, this->dir, intersection_planoBase, t_base);
    
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

    // Topo
    bool encostou_topo;
    Vec4 intersection_planoTopo;
    double t_topo;
    Vec4 centroTopo = this->centroBase + normalize(this->dir) * this->altura;
    bool encostou_planoTopo = intersect_ray_plane(origem, dir, centroTopo, this->dir, intersection_planoTopo, t_topo);

    if(!encostou_planoTopo)
    {
        encostou_topo = false;
    }
    else
    {
        double dist = (intersection_planoTopo - centroTopo).length();
        if(dist <= this->raioBase)
        {
            encostou_topo = true;
        }
        else
        {
            encostou_topo = false;
        }
    }

    // Verificando qual mostrar
    if(encostou_corpo)
    {
        if(encostou_base)
        {
            if(t_corpo < t_base)
            {
                intersection = intersection_corpo;
                t = t_corpo;
                tipoDeColisao = Corpo;
            }
            else
            {
                intersection = intersection_planoBase;
                t = t_base;
                tipoDeColisao = Base;
            }
        }
        else if(encostou_topo)
        {
            if(t_corpo < t_topo)
            {
                intersection = intersection_corpo;
                t = t_corpo;
                tipoDeColisao = Corpo;
            }
            else
            {
                intersection = intersection_planoTopo;
                t = t_topo;
                tipoDeColisao = Topo;
            }
        }
        else
        {
            intersection = intersection_corpo;
            t = t_corpo;
            tipoDeColisao = Corpo;
        }
    }
    else if (encostou_base)
    {
        if(encostou_topo)
        {
            if(t_base < t_topo)
            {
                intersection = intersection_planoBase;
                t = t_base;
                tipoDeColisao = Base;
            }
            else
            {
                intersection = intersection_planoTopo;
                t = t_topo;
                tipoDeColisao = Topo;
            }
        }
        else
        {
            intersection = intersection_planoBase;
            t = t_base;
            tipoDeColisao = Base;
        }
    }
    else if(encostou_topo)
    {
        intersection = intersection_planoTopo;
        t = t_topo;
        tipoDeColisao = Topo;
    }
    
    if(encostou_corpo || encostou_base || encostou_topo)
    {
        return true;
    }
    else
    {
        return false;
    }

}
    }

    // Método para calcular a cor com base na iluminação
    Color calculaCor(const Vec4& origem, const Vec4& intersection, const Luz& luz, const Luz& luzAmb, const Colisao& tipoDeColisao, bool isInShadow) const override {
        Vec4 n;  // Vetor normal

        switch (tipoDeColisao) {
            case Corpo: {
                // Cálculos para o corpo do cilindro
                n = normalize((intersection - centroBase) - (dir * ((intersection - centroBase).dot(dir))));  // Normal do corpo
                break;
            }
            case Base: {
                n = -dir;  // Normal da base do cilindro (invertido)
                break;
            }
            case Topo: {
                n = dir;  // Normal do topo do cilindro
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

#endif // CILINDRO_HPP
