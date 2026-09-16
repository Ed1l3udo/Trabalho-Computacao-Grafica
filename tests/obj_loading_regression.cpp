#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Malha.hpp"
#include "OBJLoader.hpp"

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory()
        : path(std::filesystem::temp_directory_path() /
               ("trabalho_obj_loading_" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

bool expect(bool condition, const std::string& message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

std::filesystem::path writeOBJ(const TemporaryDirectory& directory, const std::string& name,
                               const std::string& contents) {
    const std::filesystem::path path = directory.path / name;
    std::ofstream out(path);
    out << contents;
    return path;
}

bool contains(const std::string& text, const std::string& expected) {
    return text.find(expected) != std::string::npos;
}

bool loadFails(const std::filesystem::path& path, std::string& error) {
    std::vector<Vec4> vertices;
    std::vector<std::array<int, 3>> triangles;
    return !loadOBJ_positions_faces(path.string(), vertices, triangles, &error);
}

} // namespace

int main() {
    bool ok = true;
    const TemporaryDirectory directory;
    const std::filesystem::path missing = directory.path / "missing.obj";

    std::string error;
    ok &= expect(loadFails(missing, error) && contains(error, missing.string()) &&
                     contains(error, "nao conseguiu abrir"),
                 "arquivo inexistente deve informar o caminho e a causa");

    const std::filesystem::path empty = writeOBJ(directory, "empty.obj", "");
    ok &= expect(loadFails(empty, error) && contains(error, empty.string()) &&
                     contains(error, "nenhum vertice"),
                 "arquivo vazio deve falhar explicitamente");

    const std::filesystem::path valid = writeOBJ(directory, "valid.obj",
        "# comentario\n\n  v 0 0 0 # comentario no fim\n"
        "v 1 0 0\n v 0 1 0\n\n f 1 2 3\n");
    std::vector<Vec4> vertices;
    std::vector<std::array<int, 3>> triangles;
    error.clear();
    ok &= expect(loadOBJ_positions_faces(valid.string(), vertices, triangles, &error) &&
                     vertices.size() == 3 && triangles.size() == 1,
                 "triangulo minimo, espacos, comentarios e linhas em branco devem carregar");

    const std::filesystem::path malformedVertex = writeOBJ(directory, "bad_vertex.obj", "v 0 x 0\n");
    ok &= expect(loadFails(malformedVertex, error) && contains(error, malformedVertex.string()) &&
                     contains(error, "linha 1") && contains(error, "vertice malformado"),
                 "vertice malformado deve identificar linha e caminho");

    const std::filesystem::path malformedFace = writeOBJ(directory, "bad_face.obj",
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2\n");
    ok &= expect(loadFails(malformedFace, error) && contains(error, "linha 4") &&
                     contains(error, "ao menos tres"),
                 "face com menos de tres vertices deve falhar");

    const std::filesystem::path zeroIndex = writeOBJ(directory, "zero.obj",
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 0 1 2\n");
    ok &= expect(loadFails(zeroIndex, error) && contains(error, "linha 4") &&
                     contains(error, "indice de vertice invalido"),
                 "indice zero deve ser rejeitado");

    const std::filesystem::path outOfBounds = writeOBJ(directory, "out_of_bounds.obj",
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 4\n");
    ok &= expect(loadFails(outOfBounds, error) && contains(error, "linha 4") &&
                     contains(error, "fora dos limites"),
                 "indice positivo inexistente deve ser rejeitado");

    const std::filesystem::path negative = writeOBJ(directory, "negative.obj",
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
    error.clear();
    ok &= expect(loadOBJ_positions_faces(negative.string(), vertices, triangles, &error) &&
                     triangles.size() == 1,
                 "indices negativos validos devem continuar suportados");

    const std::filesystem::path noFaces = writeOBJ(directory, "no_faces.obj", "v 0 0 0\n");
    ok &= expect(loadFails(noFaces, error) && contains(error, noFaces.string()) &&
                     contains(error, "nenhuma face valida"),
                 "vertices sem faces devem falhar explicitamente");

    const std::filesystem::path faceFormats = writeOBJ(directory, "face_formats.obj",
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 0 1\n"
        "vn 0 0 1\n"
        "f 1 2 3\nf 1/1 2/2 3/3\nf 1//1 2//1 3//1\nf 1/1/1 2/2/1 3/3/1\n");
    error.clear();
    ok &= expect(loadOBJ_positions_faces(faceFormats.string(), vertices, triangles, &error) &&
                     triangles.size() == 4,
                 "formatos v, v/vt, v//vn e v/vt/vn devem continuar suportados");

    const std::filesystem::path polygon = writeOBJ(directory, "polygon.obj",
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");
    error.clear();
    ok &= expect(loadOBJ_positions_faces(polygon.string(), vertices, triangles, &error) &&
                     triangles.size() == 2,
                 "poligonos devem continuar triangulados em leque");

    vertices = {Vec4(9, 9, 9, 1)};
    triangles = {{0, 0, 0}};
    error.clear();
    const bool partialFailure = !loadOBJ_positions_faces(malformedFace.string(), vertices, triangles, &error);
    ok &= expect(partialFailure && vertices.size() == 1 && triangles.size() == 1 && vertices[0].x == 9,
                 "falha de carga nao pode deixar estado parcial nos vetores de saida");

    try {
        Malha mustFail(missing.string());
        (void)mustFail;
        ok &= expect(false, "Malha deve propagar a falha de carga como excecao");
    } catch (const std::runtime_error& ex) {
        ok &= expect(contains(ex.what(), missing.string()),
                     "excecao de Malha deve identificar o asset ausente");
    } catch (const std::exception&) {
        ok &= expect(false, "Malha deve usar uma excecao de erro de runtime");
    }

    return ok ? 0 : 1;
}
