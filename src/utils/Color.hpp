#ifndef COLOR_HPP
#define COLOR_HPP

class Color {
public:
    int r, g, b;  // Componentes de cor RGB (0-255)

    // Construtor com valores padrão de cor
    Color(int r = 0, int g = 0, int b = 0) : r(r), g(g), b(b) {}

    // Método para somar cores
    Color operator+(const Color& c) const {
        return Color(r + c.r, g + c.g, b + c.b);
    }

    // Método para multiplicação de cor por escalar
    Color operator*(double k) const {
        return Color(static_cast<int>(r * k), static_cast<int>(g * k), static_cast<int>(b * k));
    }

    // Método para limitar o valor de cada componente (0-255)
    Color clamp() const {
        int new_r = (r < 0) ? 0 : (r > 255) ? 255 : r;
        int new_g = (g < 0) ? 0 : (g > 255) ? 255 : g;
        int new_b = (b < 0) ? 0 : (b > 255) ? 255 : b;
        return Color(new_r, new_g, new_b);
    }
};

#endif // COLOR_HPP
