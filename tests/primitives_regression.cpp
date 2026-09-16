#include <cmath>
#include <cstdlib>
#include <iostream>

#include "Cilindro.hpp"
#include "Cone.hpp"

static constexpr double EPS = 1e-6;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Regression failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

static void expectFiniteHit(const Objeto& object, const Vec4& origin, const Vec4& direction) {
    Vec4 hit;
    double t = 0.0;
    Colisao tipo = Colisao::Nenhuma;

    require(object.intersect(origin, direction, hit, t, tipo), "expected hit");
    require(std::isfinite(t), "hit distance must be finite");
    require(std::isfinite(hit.x) && std::isfinite(hit.y) && std::isfinite(hit.z), "hit point must be finite");
}

static void expectHit(const Objeto& object, const Vec4& origin, const Vec4& direction,
                      double expectedT, Colisao expectedType) {
    Vec4 hit;
    double t = 0.0;
    Colisao tipo = Colisao::Nenhuma;
    require(object.intersect(origin, direction, hit, t, tipo), "expected hit");
    require(std::isfinite(t), "hit distance must be finite");
    require(std::isfinite(hit.x) && std::isfinite(hit.y) && std::isfinite(hit.z), "hit point must be finite");
    require(std::abs(t - expectedT) <= EPS, "unexpected nearest hit distance");
    require(tipo == expectedType, "unexpected hit type");
}

static void expectMiss(const Objeto& object, const Vec4& origin, const Vec4& direction) {
    Vec4 hit;
    double t = 0.0;
    Colisao tipo = Colisao::Nenhuma;
    require(!object.intersect(origin, direction, hit, t, tipo), "expected miss");
}

int main() {
    Cilindro cilindro(Vec4(0, 0, 0, 1), Vec4(0, 1, 0, 0), 1.0, 2.0);
    Cone cone(Vec4(0, 0, 0, 1), Vec4(0, 1, 0, 0), 1.0, 2.0);

    // Discriminante negativo: ambos devem errar sem produzir uma interseção.
    expectMiss(cilindro, Vec4(2, 1, 0, 1), Vec4(0, 0, 1, 0));
    expectMiss(cone, Vec4(2, 1, 0, 1), Vec4(0, 0, 1, 0));

    // a == 0 no cilindro: o corpo não é resolvido quadraticamente, mas a tampa é válida.
    expectHit(cilindro, Vec4(0, 3, 0, 1), Vec4(0, -1, 0, 0), 1.0, Topo);
    expectHit(cilindro, Vec4(0, -1, 0, 1), Vec4(0, 1, 0, 0), 1.0, Base);

    // a == 0 e a aproximadamente zero no cone: há solução linear válida no corpo.
    expectFiniteHit(cone, Vec4(0.9, 0, 0, 1), Vec4(-0.5, 1, 0, 0));
    expectFiniteHit(cone, Vec4(0.9, 0, 0, 1), Vec4(-0.50000001, 1, 0, 0));

    // A lateral pode não ser válida, mas a base ainda deve ser selecionada.
    expectHit(cone, Vec4(0, -1, 0, 1), Vec4(0, 1, 0, 0), 1.0, Base);

    // Acertos laterais comuns permanecem válidos.
    expectHit(cilindro, Vec4(2, 1, 0, 1), Vec4(-1, 0, 0, 0), 1.0, Corpo);
    expectHit(cone, Vec4(2, 1, 0, 1), Vec4(-1, 0, 0, 0), 1.5, Corpo);
}
