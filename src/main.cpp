#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include "Vec4.hpp"
#include "Color.hpp"
#include "Luz.hpp"
#include "Objeto.hpp"
#include "Caixa.hpp"
#include "Esfera.hpp"
#include "Cilindro.hpp"
#include "Cone.hpp"
#include "Colisao.hpp"
#include "Camera.hpp"
#include "Malha.hpp"
#include "Mat4.hpp"
#include "Texture.hpp"

// Função para calcular a cor do pixel e salvar no arquivo
void saveColor(std::ofstream& img, const Color& cor) {
    img << cor.r << ' ' << cor.g << ' ' << cor.b << ' ';
}

int main() {
    // Definindo o tamanho da imagem (500x500 pixels)
    int width = 500, height = 500;
    
    // Definindo o arquivo de saída da imagem (formato PPM)
    std::ofstream img("saida.ppm");
    img << "P3\n" << width << " " << height << "\n255\n";

    // Definindo a posição da câmera (observador)
    // Vec4 origem(0, 0, 0, 1);  // Posição da câmera no espaço
    Camera cam;
    cam.eye = Vec4{6, 6, 2, 1};   
    cam.at  = Vec4{6, 2, 8, 1};   
    cam.up  = Vec4{0, 1, 0, 0};

    cam.d = 1.0;
    cam.xmin = -1.2; cam.xmax = 1.2;
    cam.ymin = -1.2; cam.ymax = 1.2;

    
    std::vector<Luz> luzes;

    // // Definindo a luz pontual (posição e intensidade)
    // Vec4 luzPos(10, 10, 10, 1);  // Posição da luz
    // Vec4 luzIntensidade(1.0, 1.0, 1.0, 0);  // Intensidade da luz
    // Luz luzPontual(luzPos, luzIntensidade);
    Luz luzPontual(Vec4{0, 10, 0, 1}, Vec4{1, 1, 1, 0});
    luzes.push_back(luzPontual);
    
    // // Luz ambiente
    // Vec4 luzAmbiente(0.2, 0.2, 0.2, 0);
    // Luz luzAmb(luzAmbiente);
    
    Luz luzAmb(Vec4{0.2, 0.2, 0.2, 0});
    luzes.push_back(luzAmb);

    // // Criando objetos (Esfera, Cilindro, Cone)
    // Esfera esfera(Vec4(0, 0, -5, 1), 1, Vec4(0.5, 0.6, 0, 0), Vec4(1, 0.8, 0.7, 0), Vec4(0.9, 0.3, 0.4, 0), 50.0);
    // Cilindro cilindro(Vec4(2, 0, -5, 1), Vec4(0, -1, -1, 0), 1, 3, Vec4(0, 1, 0, 0), Vec4(0.5, 0.8, 0.2, 0), Vec4(0.7, 0.5, 0.8, 0), 50.0);
    // Cone cone(Vec4(-2, 0, -5, 1), Vec4(0, -1, -1, 0), 1, 3, Vec4(0, 0, 1, 0), Vec4(0.7, 0.8, 0.8, 0), Vec4(0.5, 0.7, 0.8, 0), 50.0);

    std::vector<std::unique_ptr<Objeto>> objetos;


    auto esfera = std::make_unique<Esfera>(
        Vec4(0, 0, 0, 1), 1,
        Vec4(0.5, 0.6, 0, 0), Vec4(1, 0.8, 0.7, 0), Vec4(0.9, 0.3, 0.4, 0), 50.0);
    esfera->setTransform(Mat4::translation(6, 2, 8)); // coloca no mundo (primeiro octante)

    auto texGlobo = std::make_shared<CheckerTexture>(
        Vec4{0.1, 0.4, 1.0, 0},   // azul
        Vec4{0.9, 0.9, 0.9, 0},   // branco
        24, 12                   // quantidade de quadrados
    );

    esfera->setTexture(texGlobo);

    auto cilindro = std::make_unique<Cilindro>(
        Vec4(0, 0, 0, 1), Vec4(0, 1, 0, 0), 1, 3,
        Vec4(0, 1, 0, 0), Vec4(0.5, 0.8, 0.2, 0), Vec4(0.7, 0.5, 0.8, 0), 50.0);
    cilindro->setTransform(Mat4::translation(9, 1, 8) * Mat4::rotateZ(0.3)); // translação + rotação

    auto cone = std::make_unique<Cone>(
        Vec4(0, 0, 0, 1), Vec4(0, 1, 0, 0), 1, 3,
        Vec4(0, 0, 1, 0), Vec4(0.7, 0.8, 0.8, 0), Vec4(0.5, 0.7, 0.8, 0), 50.0);
    cone->setTransform(Mat4::translation(3, 1, 8) * Mat4::shear(0.2,0, 0,0, 0,0)); // cisalhamento exemplo
    
    objetos.push_back(std::move(esfera));
    objetos.push_back(std::move(cilindro));
    objetos.push_back(std::move(cone));

    auto chao = std::make_unique<Caixa>(
        Vec4(0,0,0,1), Vec4(1,1,1,1),
        Vec4(0.2,0.2,0.2,0),   // Ke
        Vec4(0.7,0.7,0.7,0),   // Kd
        Vec4(0.08,0.08,0.08,0),// Ka
        30.0
    );

    // transforma um cubo unitário em uma “laje” grande
    chao->setTransform(
        Mat4::translation(1, 0.5, 1) *   // coloca no mundo
        Mat4::scale(12, 0.2, 12)         // vira piso
    );

    objetos.push_back(std::move(chao));

    auto teclado = std::make_unique<Malha>(
        "models/teclado.obj",
        Vec4{0.25,0.25,0.25,0},   // Ke (spec)
        Vec4{0.15,0.15,0.15,0},   // Kd (difuso)
        Vec4{0.06,0.06,0.06,0},   // Ka (ambiente)
        40.0
    );

    // colocar em cima da mesa (tudo positivo)
    teclado->setTransform(
        Mat4::translation(4.5, 1.25, 6.5) *  // ajuste a altura conforme sua mesa
        Mat4::scale(1.0, 1.0, 1.0)
    );

    objetos.push_back(std::move(teclado));


    // Configurações de imagem (janela de visualização)
    double wJanela = 60;
    double hJanela = 60;
    double Dx = wJanela / width;
    double Dy = hJanela / height;
    Vec4 centroJanela(0, 0, -30, 1);  // Centro da janela

    // Loop para renderizar a imagem
    for (int l = 0; l < height; ++l) {
        for (int c = 0; c < width; ++c) {
            Vec4 origem, rayDir;
            cam.geraRaio(c, l, width, height, origem, rayDir);

            // Variáveis para armazenar as interseções
            Vec4 intersection;
            double t;
            bool tocouObjeto = false;
            Color corFinal(0, 0, 0);
            Colisao colisaoEsfera;
            Colisao colisaoCone;
            Colisao colisaoCilindro;

            // // Verificando interseções com a Esfera
            // if (esfera.intersect(origem, rayDir, intersection, t, colisaoEsfera)) {
            //     corFinal = esfera.calculaCor(origem, intersection, luzPontual, luzAmbiente, Colisao::Nenhuma, false);
            //     tocouObjeto = true;
            // }

            // // Verificando interseções com o Cilindro
            // if (cilindro.intersect(origem, rayDir, intersection, t, colisaoCilindro)) {
            //     corFinal = cilindro.calculaCor(origem, intersection, luzPontual, luzAmbiente, colisaoCilindro, false);
            //     tocouObjeto = true;
            // }

            // // Verificando interseções com o Cone
            // if (cone.intersect(origem, rayDir, intersection, t, colisaoCone)) {
            //     corFinal = cone.calculaCor(origem, intersection, luzPontual, luzAmbiente, colisaoCone, false);
            //     tocouObjeto = true;
            // }

            // // Se não houve interseção, cor de fundo (cinza)
            // if (!tocouObjeto) {
            //     corFinal = Color(100, 100, 100);
            // }

            double tMin = 1e18;
            Objeto* objHit = nullptr;
            Vec4 pHit;
            Colisao tipoHit = Colisao::Nenhuma; // ajuste se seu enum não tiver

            for (auto& obj : objetos) {
                Vec4 p;
                double t;
                Colisao tipo;

                if (obj->intersect(origem, rayDir, p, t, tipo)) {
                    if (t > 1e-6 && t < tMin) {
                        tMin = t;
                        objHit = obj.get();
                        pHit = p;
                        tipoHit = tipo;
                    }
                }
            }

            
            bool isInShadow = false; 
            if (objHit) {
                Vec4 P = pHit;

                Vec4 toLight = luzPontual.pos - P;
                double distToLight = toLight.length();
                Vec4 shadowDir = normalize(toLight);

                // empurra um pouquinho para evitar "self-shadow"
                Vec4 shadowOrigin = P + shadowDir * 1e-3;

                for (auto& obj : objetos) {
                    if (obj.get() == objHit) continue; // evita auto-sombra

                    Vec4 pS;
                    double tS;
                    Colisao tipoS;

                    if (obj->intersect(shadowOrigin, shadowDir, pS, tS, tipoS)) {
                        if (tS > 1e-6 && tS < distToLight - 1e-4) {
                            isInShadow = true;
                            break;
                        }
                    }
                }

            corFinal = objHit->calculaCor(origem, pHit, luzPontual, luzAmb, tipoHit, isInShadow);
            }

            // Salvar a cor no arquivo
            saveColor(img, corFinal);
        }
        img << '\n';
    }

    img.close();  // Fecha o arquivo de imagem
    std::cout << "Imagem gerada com sucesso (saida.ppm)." << std::endl;
    return 0;
}

