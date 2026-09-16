#pragma once
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <exception>
#include <limits>

#include "Vec4.hpp"

// Lê OBJ mínimo:
// - v x y z [w]
// - f a b c [d ...]  (triangula em fan)
// Suporta tokens v, v/vt, v//vn e v/vt/vn; usa somente o índice de v.
static inline bool loadOBJ_positions_faces(
    const std::string& filename,
    std::vector<Vec4>& outVertices,
    std::vector<std::array<int,3>>& outTris,
    std::string* err = nullptr
) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        if (err) *err = "OBJ '" + filename + "': nao conseguiu abrir o arquivo.";
        return false;
    }

    std::vector<Vec4> vertices;
    std::vector<std::array<int,3>> tris;

    auto fail = [&](std::size_t lineNumber, const std::string& reason) {
        if (err) {
            *err = "OBJ '" + filename + "', linha " + std::to_string(lineNumber) + ": " + reason;
        }
        return false;
    };

    auto parseIndex = [](const std::string& token, int& value) {
        try {
            std::size_t parsed = 0;
            const long long parsedValue = std::stoll(token, &parsed);
            if (parsed != token.size() || parsedValue == 0 ||
                parsedValue < std::numeric_limits<int>::min() ||
                parsedValue > std::numeric_limits<int>::max()) {
                return false;
            }
            value = static_cast<int>(parsedValue);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    };

    auto parseFaceToken = [&](const std::string& token, int& vertexIndex, std::string& reason) {
        const std::size_t firstSlash = token.find('/');
        const std::string vertexToken = token.substr(0, firstSlash);
        if (!parseIndex(vertexToken, vertexIndex)) {
            reason = "indice de vertice invalido na face.";
            return false;
        }

        if (firstSlash == std::string::npos) return true;

        const std::size_t secondSlash = token.find('/', firstSlash + 1);
        const std::string textureToken = token.substr(
            firstSlash + 1,
            secondSlash == std::string::npos ? std::string::npos : secondSlash - firstSlash - 1);
        if (!textureToken.empty()) {
            int ignored = 0;
            if (!parseIndex(textureToken, ignored)) {
                reason = "indice de textura invalido na face.";
                return false;
            }
        }

        if (secondSlash == std::string::npos) {
            if (textureToken.empty()) {
                reason = "formato de face invalido.";
                return false;
            }
            return true;
        }

        if (token.find('/', secondSlash + 1) != std::string::npos) {
            reason = "formato de face invalido.";
            return false;
        }

        const std::string normalToken = token.substr(secondSlash + 1);
        if (normalToken.empty()) {
            reason = "formato de face invalido.";
            return false;
        }

        int ignored = 0;
        if (!parseIndex(normalToken, ignored)) {
            reason = "indice de normal invalido na face.";
            return false;
        }
        return true;
    };

    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);

        std::istringstream ss(line);
        std::string kind;
        if (!(ss >> kind)) continue;

        // vertex
        if (kind == "v") {
            double x, y, z;
            if (!(ss >> x >> y >> z) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
                return fail(lineNumber, "vertice malformado.");
            }

            double ignoredW = 1.0;
            if (ss >> ignoredW) {
                if (!std::isfinite(ignoredW)) return fail(lineNumber, "vertice malformado.");

                std::string extra;
                if (ss >> extra) return fail(lineNumber, "vertice malformado.");
            } else if (!ss.eof()) {
                return fail(lineNumber, "vertice malformado.");
            }

            vertices.push_back(Vec4(x, y, z, 1));
            continue;
        }

        // face
        if (kind == "f") {
            std::vector<int> idx;
            std::string tok;
            while (ss >> tok) {
                int vi = 0;
                std::string reason;
                if (!parseFaceToken(tok, vi, reason)) return fail(lineNumber, reason);

                const int realIndex = vi > 0 ? vi - 1 : static_cast<int>(vertices.size()) + vi;
                if (realIndex < 0 || realIndex >= static_cast<int>(vertices.size())) {
                    return fail(lineNumber, "indice de vertice fora dos limites na face.");
                }
                idx.push_back(realIndex);
            }

            if (idx.size() < 3) return fail(lineNumber, "face precisa de ao menos tres vertices.");

            // triangula fan: (0,i,i+1)
            for (size_t i=1; i+1<idx.size(); i++) {
                tris.push_back({ idx[0], idx[i], idx[i+1] });
            }
        }
    }

    if (vertices.empty()) {
        if (err) *err = "OBJ '" + filename + "': nenhum vertice foi encontrado.";
        return false;
    }
    if (tris.empty()) {
        if (err) *err = "OBJ '" + filename + "': nenhuma face valida foi encontrada.";
        return false;
    }

    outVertices = std::move(vertices);
    outTris = std::move(tris);
    return true;
}
