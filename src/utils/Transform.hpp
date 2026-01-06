#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "Vec4.hpp"
#include <cmath>  // Para funções trigonométricas e outras operações matemáticas

// Produto Hadamard (componente a componente) - já implementado
static inline Vec4 hadamard(const Vec4& a, const Vec4& b) {
    return Vec4(a.x * b.x, a.y * b.y, a.z * b.z, 0);  // O componente w não é utilizado no Hadamard
}

// Normalizar um vetor (transformação de normalização)
static inline Vec4 normalize(const Vec4& v) {
    double L = v.length();
    if (L == 0.0) return v;
    return v / L;
}

static inline int clamp255(double v) {
    if (v < 0.0)   return 0;
    if (v > 255.0) return 255;
    return static_cast<int>(v + 0.5); // arredonda
}

#endif // TRANSFORM_HPP
