#ifndef OBJETO_HPP
#define OBJETO_HPP

#include "Vec4.hpp"  // Incluindo o header que contém a definição de Vec4
#include "Color.hpp"  // Incluindo o header de Color
#include "Luz.hpp"    // Incluindo o header de Luz
#include "Colisao.hpp" // Inclui a enumeração Colisao

class Objeto {
public:
    // Método para verificar a interseção com um raio
    virtual bool intersect(const Vec4& origem, const Vec4& dir, Vec4& intersection, double& t) const = 0;

    // Versão do método calculaCor com tipoDeColisao (para objetos que precisam desse parâmetro)
    virtual Color calculaCor(const Vec4& origem, const Vec4& intersection, const Luz& luz, const Luz& luzAmb, const Colisao& tipoDeColisao, bool isInShadow) const = 0;

    // Versão do método calculaCor sem tipoDeColisao (para objetos que não precisam desse parâmetro)
    virtual Color calculaCor(const Vec4& origem, const Vec4& intersection, const Luz& luz, const Luz& luzAmb, bool isInShadow) const {
        return calculaCor(origem, intersection, luz, luzAmb, Colisao::Nenhuma, isInShadow);  // Chama a versão com tipoDeColisao com o valor padrão
    }

    virtual ~Objeto() = default;  // Destruidor virtual
};

#endif // OBJETO_HPP
