#ifndef LUZ_HPP
#define LUZ_HPP

#include "Vec4.hpp"  // Inclui a definição do vetor 4D (para posição da luz e intensidade)

class Luz {
public:
    Vec4 pos;          // Posição da luz (para luzes pontuais)
    Vec4 intensidade;  // Intensidade da luz (para luzes pontuais, direcional, etc.)

    // Construtor para luz pontual (com posição e intensidade)
    Luz(const Vec4& pos, const Vec4& intensidade) : pos(pos), intensidade(intensidade) {}

    // Construtor para luz ambiente (sem posição, só intensidade)
    Luz(const Vec4& intensidade) : pos(0, 0, 0, 1), intensidade(intensidade) {}

    // Método para aplicar a intensidade da luz
    Vec4 intensidadeComLuz(const Vec4& cor) const {
        return intensidade * cor.length();  // Intensidade modulada pela cor do objeto
    }
};

#endif // LUZ_HPP
