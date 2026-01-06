#ifndef VEC4_HPP
#define VEC4_HPP

#include <cmath>  // Para sqrt

// Classe para representar um vetor 4D (usado para coordenadas, vetores, etc)
class Vec4 {
public:
    double x, y, z, w;  // Coordenadas do vetor (x, y, z, w)

    // Construtor padrão
    Vec4(double x=0, double y=0, double z=0, double w=1) : x(x), y(y), z(z), w(w) {}

    // Operadores de sobrecarga para operações vetoriais
    Vec4 operator+(const Vec4& v) const { return Vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vec4 operator-(const Vec4& v) const { return Vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vec4 operator*(double k) const { return Vec4(x * k, y * k, z * k, w); }
    Vec4 operator/(double k) const { return Vec4(x / k, y / k, z / k, w); }
    Vec4 operator-() const { return Vec4(-x, -y, -z, w); }

    // Produto escalar (dot product)
    double dot(const Vec4& v) const { return x * v.x + y * v.y + z * v.z; }

    // Comprimento (magnitude) do vetor
    double length() const { return std::sqrt(x * x + y * y + z * z); }

    // Normalizar o vetor (tornar o comprimento igual a 1)
    Vec4 normalize() const {
        double len = length();
        if (len == 0.0) return *this;  // Evita divisão por zero
        return *this / len;
    }
};

#endif // VEC4_HPP
