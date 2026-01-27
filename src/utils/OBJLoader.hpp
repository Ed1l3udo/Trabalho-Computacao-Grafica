#pragma once
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>

#include "Vec4.hpp"

// Lê OBJ mínimo:
// - v x y z
// - f a b c [d ...]  (triangula em fan)
// Suporta tokens tipo "f 1/2/3 4/5/6 7/8/9" (usa só o índice de v).
static inline bool loadOBJ_positions_faces(
    const std::string& filename,
    std::vector<Vec4>& outVertices,
    std::vector<std::array<int,3>>& outTris,
    std::string* err = nullptr
) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        if (err) *err = "Nao conseguiu abrir OBJ: " + filename;
        return false;
    }

    outVertices.clear();
    outTris.clear();

    std::string line;
    while (std::getline(in, line)) {
        if (line.size() < 2) continue;

        // vertex
        if (line.rfind("v ", 0) == 0) {
            std::istringstream ss(line);
            char vch;
            double x,y,z;
            ss >> vch >> x >> y >> z;
            outVertices.push_back(Vec4(x,y,z,1));
            continue;
        }

        // face
        if (line.rfind("f ", 0) == 0) {
            std::istringstream ss(line);
            char fch;
            ss >> fch;

            std::vector<int> idx;
            std::string tok;
            while (ss >> tok) {
                // pega parte antes do '/'
                size_t slash = tok.find('/');
                std::string a = (slash == std::string::npos) ? tok : tok.substr(0, slash);
                if (a.empty()) continue;

                int vi = std::stoi(a); // OBJ é 1-based (ou negativo)
                int realIndex = -1;

                if (vi > 0) realIndex = vi - 1;
                else        realIndex = (int)outVertices.size() + vi; // vi negativo

                if (realIndex < 0 || realIndex >= (int)outVertices.size()) {
                    if (err) *err = "Indice de vertice invalido em face (OBJ).";
                    return false;
                }
                idx.push_back(realIndex);
            }

            if (idx.size() < 3) continue;

            // triangula fan: (0,i,i+1)
            for (size_t i=1; i+1<idx.size(); i++) {
                outTris.push_back({ idx[0], idx[i], idx[i+1] });
            }
        }
    }

    if (outVertices.empty() || outTris.empty()) {
        if (err) *err = "OBJ sem vertices ou sem faces: " + filename;
        return false;
    }

    return true;
}
