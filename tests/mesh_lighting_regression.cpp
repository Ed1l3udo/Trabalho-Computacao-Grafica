#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "Malha.hpp"
#include "Esfera.hpp"

namespace {

bool isFinite(const Vec4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z) && std::isfinite(value.w);
}

bool isValidColor(const Color& color) {
    return color.r >= 0 && color.r <= 255 &&
           color.g >= 0 && color.g <= 255 &&
           color.b >= 0 && color.b <= 255;
}

bool sameColor(const Color& a, const Color& b, int tolerance = 1) {
    return std::abs(a.r - b.r) <= tolerance &&
           std::abs(a.g - b.g) <= tolerance &&
           std::abs(a.b - b.b) <= tolerance;
}

class TemporaryTriangleOBJ {
public:
    TemporaryTriangleOBJ()
        : path(std::filesystem::temp_directory_path() /
               ("mesh_lighting_regression_" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".obj")) {
        std::ofstream out(path);
        out << "v -1 0 -1\n"
               "v 1 0 -1\n"
               "v 0 0 1\n"
               "f 1 2 3\n";
    }

    ~TemporaryTriangleOBJ() {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }

    std::filesystem::path path;
};

Malha makeUpwardFacingMesh(const std::string& objPath) {
    Malha mesh(objPath,
               Vec4(0, 0, 0, 0), Vec4(1, 1, 1, 0), Vec4(0, 0, 0, 0), 8.0);
    mesh.vertices = {Vec4(-1, 0, -1, 1), Vec4(1, 0, -1, 1), Vec4(0, 0, 1, 1)};
    mesh.tris = {{0, 1, 2}};
    mesh.lastNormalLocal = Vec4(0, 1, 0, 0);
    return mesh;
}

bool expect(bool condition, const char* message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

} // namespace

int main() {
    bool ok = true;
    const Vec4 camera(0, 5, 0, 1);
    const Vec4 point(0, 0, 0, 1);
    const Luz ambient(Vec4(0, 0, 0, 0));
    const TemporaryTriangleOBJ triangle;
    Malha mesh = makeUpwardFacingMesh(triangle.path.string());

    const Luz directional = Luz::Direcional(Vec4(0, -5, 0, 0), Vec4(1, 1, 1, 0));
    const Color directionalLit = mesh.calculaCor(camera, point, directional, ambient,
                                                   Colisao::Corpo, false);
    ok &= expect(directionalLit.r > 250 && directionalLit.g > 250 && directionalLit.b > 250,
                 "a face voltada para a luz direcional deve receber difuso");

    const Luz directionalOpposite = Luz::Direcional(Vec4(0, 5, 0, 0), Vec4(1, 1, 1, 0));
    const Color directionalUnlit = mesh.calculaCor(camera, point, directionalOpposite, ambient,
                                                     Colisao::Corpo, false);
    ok &= expect(directionalUnlit.r == 0 && directionalUnlit.g == 0 && directionalUnlit.b == 0,
                 "a face oposta nao deve receber difuso direcional");

    const Color directionalAtBelow = mesh.calculaCor(camera, Vec4(0, -1, 0, 1), directional,
                                                       ambient, Colisao::Corpo, false);
    const Color directionalAtAbove = mesh.calculaCor(camera, Vec4(0, 1, 0, 1), directional,
                                                       ambient, Colisao::Corpo, false);
    ok &= expect(sameColor(directionalAtBelow, directionalAtAbove),
                 "luz direcional nao pode variar com a posicao do ponto");

    const Luz directionalUnit = Luz::Direcional(Vec4(0, -1, 0, 0), Vec4(1, 1, 1, 0));
    const Color directionalUnitColor = mesh.calculaCor(camera, point, directionalUnit, ambient,
                                                        Colisao::Corpo, false);
    ok &= expect(sameColor(directionalLit, directionalUnitColor),
                 "direcao direcional nao normalizada deve ser normalizada internamente");
    ok &= expect(isFinite(directional.Lvec(point)) && isValidColor(directionalLit) &&
                 isValidColor(directionalUnlit),
                 "iluminacao direcional deve produzir valores finitos");

    const Luz spot = Luz::Spot(Vec4(0, 2, 0, 1), Vec4(0, -4, 0, 0), 30.0,
                                Vec4(1, 1, 1, 0));
    const Color spotCenter = mesh.calculaCor(camera, point, spot, ambient, Colisao::Corpo, false);
    const Color spotInside = mesh.calculaCor(camera, Vec4(0.5, 0, 0, 1), spot, ambient,
                                              Colisao::Corpo, false);
    const Color spotOutside = mesh.calculaCor(camera, Vec4(3, 0, 0, 1), spot, ambient,
                                               Colisao::Corpo, false);
    const Vec4 nearInside(1.14, 0, 0, 1);  // pouco dentro de 30 graus a partir de (0, 2, 0)
    const Vec4 nearOutside(1.17, 0, 0, 1); // pouco fora do mesmo cone

    ok &= expect(spotCenter.r > 250 && spotInside.r > 0,
                 "centro e interior do cone do spot devem receber luz");
    ok &= expect(spotOutside.r == 0 && spotOutside.g == 0 && spotOutside.b == 0,
                 "ponto fora do cone do spot deve receber apenas ambiente");
    ok &= expect(spot.factorAt(nearInside) == 1.0 && spot.factorAt(nearOutside) == 0.0,
                 "a fronteira do spot deve usar cutoff em graus convertido para cosseno");
    ok &= expect(isFinite(spot.Lvec(point)) && isValidColor(spotCenter) &&
                 isValidColor(spotInside) && isValidColor(spotOutside),
                 "iluminacao do spot deve produzir valores finitos");

    Esfera sphere(Vec4(0, -1, 0, 1), 1.0, Vec4(0, 0, 0, 0),
                  Vec4(1, 1, 1, 0), Vec4(0, 0, 0, 0), 8.0);
    const Color sphereDirectional = sphere.calculaCor(camera, point, directional, ambient,
                                                       Colisao::Corpo, false);
    ok &= expect(sameColor(directionalLit, sphereDirectional),
                 "malha e esfera equivalentes devem ter a mesma iluminacao direcional");

    return ok ? 0 : 1;
}
