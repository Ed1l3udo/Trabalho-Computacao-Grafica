#include <iostream>
#include <fstream>
#include "Vec4.hpp"
#include "Color.hpp"
#include "Luz.hpp"
#include "Objeto.hpp"
#include "Esfera.hpp"
#include "Cilindro.hpp"
#include "Cone.hpp"
#include "Colisao.hpp"

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
    Vec4 origem(0, 0, 0, 1);  // Posição da câmera no espaço

    // Definindo a luz pontual (posição e intensidade)
    Vec4 luzPos(10, 10, 10, 1);  // Posição da luz
    Vec4 luzIntensidade(1.0, 1.0, 1.0, 0);  // Intensidade da luz
    Luz luzPontual(luzPos, luzIntensidade);
    
    // Luz ambiente
    Vec4 luzAmbiente(0.2, 0.2, 0.2, 0);
    Luz luzAmb(luzAmbiente);
    
    // Criando objetos (Esfera, Cilindro, Cone)
    Esfera esfera(Vec4(0, 0, -5, 1), 1, Vec4(0.5, 0.6, 0, 0), Vec4(1, 0.8, 0.7, 0), Vec4(0.9, 0.3, 0.4, 0), 50.0);
    Cilindro cilindro(Vec4(2, 0, -5, 1), Vec4(0, -1, -1, 0), 1, 3, Vec4(0, 1, 0, 0), Vec4(0.5, 0.8, 0.2, 0), Vec4(0.7, 0.5, 0.8, 0), 50.0);
    Cone cone(Vec4(-2, 0, -5, 1), Vec4(0, -1, -1, 0), 1, 3, Vec4(0, 0, 1, 0), Vec4(0.7, 0.8, 0.8, 0), Vec4(0.5, 0.7, 0.8, 0), 50.0);

    // Configurações de imagem (janela de visualização)
    double wJanela = 60;
    double hJanela = 60;
    double Dx = wJanela / width;
    double Dy = hJanela / height;
    Vec4 centroJanela(0, 0, -30, 1);  // Centro da janela

    // Loop para renderizar a imagem
    for (int l = 0; l < height; ++l) {
        for (int c = 0; c < width; ++c) {
            double x = -wJanela / 2 + Dx / 2 + c * Dx;
            double y = hJanela / 2 - Dy / 2 - l * Dy;
            Vec4 pixel(x, y, centroJanela.z, 1);  // Posição do pixel na janela

            // Raio da câmera (origem -> pixel)
            Vec4 ray = pixel - origem;
            Vec4 rayDir = normalize(ray);

            // Variáveis para armazenar as interseções
            Vec4 intersection;
            double t;
            bool tocouObjeto = false;
            Color corFinal(0, 0, 0);
            Colisao colisaoEsfera;
            Colisao colisaoCone;
            Colisao colisaoCilindro;

            // Verificando interseções com a Esfera
            if (esfera.intersect(origem, rayDir, intersection, t, colisaoEsfera)) {
                corFinal = esfera.calculaCor(origem, intersection, luzPontual, luzAmbiente, Colisao::Nenhuma, false);
                tocouObjeto = true;
            }

            // Verificando interseções com o Cilindro
            if (cilindro.intersect(origem, rayDir, intersection, t, colisaoCilindro)) {
                corFinal = cilindro.calculaCor(origem, intersection, luzPontual, luzAmbiente, colisaoCilindro, false);
                tocouObjeto = true;
            }

            // Verificando interseções com o Cone
            if (cone.intersect(origem, rayDir, intersection, t, colisaoCone)) {
                corFinal = cone.calculaCor(origem, intersection, luzPontual, luzAmbiente, colisaoCone, false);
                tocouObjeto = true;
            }

            // Se não houve interseção, cor de fundo (cinza)
            if (!tocouObjeto) {
                corFinal = Color(100, 100, 100);
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
