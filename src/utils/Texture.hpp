#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Vec4.hpp"

// Interface de textura: retorna cor em [0..1]
class Texture {
public:
    virtual ~Texture() = default;
    virtual Vec4 sample(double u, double v) const = 0;
};

// Checkerboard procedural (não precisa arquivo)
class CheckerTexture : public Texture {
public:
    Vec4 c1, c2;      // cores em [0..1], w=0
    int freqU, freqV; // quantos quadrados

    CheckerTexture(Vec4 c1 = Vec4(1,1,1,0), Vec4 c2 = Vec4(0,0,0,0), int freqU = 12, int freqV = 6)
        : c1(c1), c2(c2), freqU(freqU), freqV(freqV) {}

    Vec4 sample(double u, double v) const override {
        // wrap [0,1)
        u = u - std::floor(u);
        v = v - std::floor(v);

        int iu = (int)std::floor(u * freqU);
        int iv = (int)std::floor(v * freqV);
        bool odd = ((iu + iv) & 1) != 0;
        return odd ? c2 : c1;
    }
};

// Textura por imagem PPM (P3 ou P6) - opcional, caso o professor exija "imagem"
class ImageTexturePPM : public Texture {
public:
    int w = 0, h = 0;
    std::vector<Vec4> px; // RGB em [0..1], w=0

    ImageTexturePPM() = default;
    explicit ImageTexturePPM(const std::string& path) { load(path); }

    void load(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Nao abriu textura PPM: " + path);

        std::string magic;
        in >> magic;
        if (magic != "P3" && magic != "P6") throw std::runtime_error("PPM invalido (so P3/P6): " + path);

        auto skip_comments = [&]() {
            while (in >> std::ws && in.peek() == '#') {
                std::string dummy;
                std::getline(in, dummy);
            }
        };

        skip_comments();
        in >> w;
        skip_comments();
        in >> h;
        skip_comments();
        int maxv;
        in >> maxv;
        if (w <= 0 || h <= 0 || maxv <= 0) throw std::runtime_error("PPM cabecalho invalido: " + path);

        px.assign((size_t)w * (size_t)h, Vec4(0,0,0,0));

        if (magic == "P3") {
            for (int i = 0; i < w*h; i++) {
                int r,g,b;
                in >> r >> g >> b;
                px[i] = Vec4(r/(double)maxv, g/(double)maxv, b/(double)maxv, 0);
            }
        } else {
            // P6: precisa consumir 1 byte de whitespace após o header
            in.get();
            for (int i = 0; i < w*h; i++) {
                unsigned char rgb[3];
                in.read((char*)rgb, 3);
                px[i] = Vec4(rgb[0]/(double)maxv, rgb[1]/(double)maxv, rgb[2]/(double)maxv, 0);
            }
        }
    }

    Vec4 sample(double u, double v) const override {
        if (w <= 0 || h <= 0 || px.empty()) return Vec4(1,0,1,0); // magenta = erro visível

        // wrap
        u = u - std::floor(u);
        v = v - std::floor(v);

        // v=0 no "baixo" -> converte para coordenada de imagem (y=0 topo)
        int x = std::clamp((int)std::floor(u * w), 0, w-1);
        int y = std::clamp((int)std::floor((1.0 - v) * h), 0, h-1);

        return px[(size_t)y * (size_t)w + (size_t)x];
    }
};
