#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>

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
#include "Quaternion.hpp"

// Salva cor no PPM
static inline void saveColor(std::ofstream& img, const Color& cor) {
    img << cor.r << ' ' << cor.g << ' ' << cor.b << ' ';
}

int main() {
    // Imagem
    const int width = 500, height = 500;

    std::ofstream img("saida.ppm");
    img << "P3\n" << width << " " << height << "\n255\n";

    // Câmera
    Camera cam;
    cam.eye = Vec4{0, 5, 1, 1};
    cam.at  = Vec4{6, 2, 8, 1};
    cam.up  = Vec4{0, 1, 0, 0};
    cam.d = 1.0;
    cam.xmin = -1.2; cam.xmax = 1.2;
    cam.ymin = -1.2; cam.ymax = 1.2;

    // Luzes (ray casting offline: usa uma pontual + ambiente)
    Luz luzPontual(Vec4{10, 10, 2, 1}, Vec4{1,1,1,0});
    Luz luzAmb(Vec4{0.2,0.2,0.2,0});

    // Cena
    std::vector<std::unique_ptr<Objeto>> objetos;

    // ---------------- CHÃO ----------------
    Vec4 KeFloor{0.02,0.02,0.02,0};
    Vec4 KdFloor{0.35,0.30,0.25,0};
    Vec4 KaFloor{0.10,0.08,0.06,0};
    double mFloor = 15.0;

    double roomX = 14.0;
    double roomZ = 12.0;
    double wallT = 0.20;

    double rx0 = -1.0;
    double rz0 = 0.5;

    {
        auto floor = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KeFloor, KdFloor, KaFloor, mFloor
        );
        floor->setTransform(
            Mat4::translation(rx0, 0.0, rz0) *
            Mat4::scale(roomX, wallT, roomZ)
        );
        objetos.push_back(std::move(floor));
    }

    // ---------------- MESA (tampo + 4 pernas) ----------------
    Vec4 KeMadeira{0.05, 0.05, 0.05, 0};
    Vec4 KdMadeira{0.55, 0.32, 0.15, 0};
    Vec4 KaMadeira{0.10, 0.06, 0.03, 0};
    double mMadeira = 10.0;

    Vec4 KePreto{0.25, 0.25, 0.25, 0};
    Vec4 KdPreto{0.05, 0.05, 0.05, 0};
    Vec4 KaPreto{0.02, 0.02, 0.02, 0};
    double mPreto = 80.0;

    double mesaX = 10.0;
    double mesaZ = 6.0;
    double tampoEsp = 0.25;
    double altura = 2.0;
    double pernaEsp = 0.20;

    double x0 = 1.0;
    double z0 = 3.0;

    {
        auto tampo = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KeMadeira, KdMadeira, KaMadeira, mMadeira
        );
        Mat4 T = Mat4::translation(x0, altura, z0) * Mat4::scale(mesaX, tampoEsp, mesaZ);
        tampo->setTransform(T);
        objetos.push_back(std::move(tampo));
    }

    auto criaPerna = [&](double px, double pz) {
        auto perna = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KePreto, KdPreto, KaPreto, mPreto
        );
        Mat4 T = Mat4::translation(px, 0.0, pz) * Mat4::scale(pernaEsp, altura, pernaEsp);
        perna->setTransform(T);
        objetos.push_back(std::move(perna));
    };

    double inset = 0.35;
    double px1 = x0 + inset;
    double px2 = x0 + mesaX - inset - pernaEsp;
    double pz1 = z0 + inset;
    double pz2 = z0 + mesaZ - inset - pernaEsp;

    criaPerna(px1, pz1);
    criaPerna(px2, pz1);
    criaPerna(px1, pz2);
    criaPerna(px2, pz2);

    // ---------------- LÁPIS (cilindro + cone) ----------------
    Vec4 KePencil{0.15,0.15,0.15,0};
    Vec4 KdPencil{0.95,0.80,0.20,0};
    Vec4 KaPencil{0.10,0.08,0.02,0};
    double mPencil = 40.0;

    Vec4 KeTip{0.10,0.10,0.10,0};
    Vec4 KdTip{0.85,0.65,0.45,0};
    Vec4 KaTip{0.08,0.06,0.04,0};
    double mTip = 25.0;

    double pencilX = 8.5;
    double pencilY = (altura + tampoEsp) + 0.03;
    double pencilZ = 4.0;

    double pencilLen = 1.2;
    double pencilR = 0.06;

    double tipLen = 0.25;
    double tipR = 0.07;

    Mat4 Rdeitar = Mat4::rotateX(3.14159265358979323846/2.0);

    {
        auto corpo = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KePencil, KdPencil, KaPencil, mPencil
        );
        corpo->setTransform(
            Mat4::translation(pencilX, pencilY, pencilZ) *
            Rdeitar *
            Mat4::scale(pencilR, pencilLen, pencilR)
        );
        objetos.push_back(std::move(corpo));
    }

    {
        auto ponta = std::make_unique<Cone>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeTip, KdTip, KaTip, mTip
        );
        ponta->setTransform(
            Mat4::translation(pencilX, pencilY, pencilZ) *
            Rdeitar *
            Mat4::translation(0, pencilLen, 0) *
            Mat4::scale(tipR, tipLen, tipR)
        );
        objetos.push_back(std::move(ponta));
    }

    // ---------------- BORRACHA + ESPELHO em x = centro da mesa ----------------
    double xCenterMesa = x0 + mesaX * 0.5;
    Mat4 espelhoMesaX = Mat4::mirrorPlane(Vec4(xCenterMesa, 0, 0, 1), Vec4(1, 0, 0, 0));

    Vec4 KeE{0.05,0.05,0.05,0};
    Vec4 KdE{0.95,0.95,0.95,0};
    Vec4 KaE{0.25,0.25,0.25,0};
    double mE = 20.0;

    double yawDeg = 110.0;
    double yawRad = yawDeg * 3.14159265358979323846 / 180.0;
    Mat4 Rer = Mat4::rotateY(yawRad);

    double erL = 0.30, erH = 0.12, erW = 0.18;

    double erCX = x0 + 8.2;
    double erCY = (altura + tampoEsp) + 0.02 + erH*0.5;
    double erCZ = z0 + 1.6;

    Mat4 Ter =
        Mat4::translation(erCX, erCY, erCZ) *
        Rer *
        Mat4::scale(erL, erH, erW) *
        Mat4::translation(-0.5, -0.5, -0.5);

    {
        auto borracha1 = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KeE, KdE, KaE, mE
        );
        borracha1->setTransform(Ter);
        objetos.push_back(std::move(borracha1));
    }

    {
        auto borracha2 = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KeE, KdE, KaE, mE
        );
        Mat4 Ter2 = espelhoMesaX * Ter;
        borracha2->setTransform(Ter2);
        objetos.push_back(std::move(borracha2));
    }

    //  PILHA DE PAPÉIS (caixa branca com CISALHAMENTO) 

    // material branco (papel)
    Vec4 KePaper{0.02,0.02,0.02,0};
    Vec4 KdPaper{0.95,0.95,0.95,0};
    Vec4 KaPaper{0.25,0.25,0.25,0};
    double mPaper = 8.0;

    // dimensões da pilha
    double papL = 1.4;  // comprimento em X
    double papH = 0.18;  // altura em Y
    double papW = 2;  // largura em Z

    // posição (em cima da mesa)
    double papCX = x0 + 3.2;
    double papCY = altura + tampoEsp + 0.02 + papH*0.5;
    double papCZ = z0 + 4.6;

    // cisalhamento: x += k*y (empurra topo pro lado +X)
    double kShear = 0.8;  

    Mat4 Shear = Mat4::shear(kShear, 0,   // shXY, shXZ
                            0,      0,   // shYX, shYZ
                            0,      0);  // shZX, shZY

    
    double yaw = 10.0 * 3.14159265358979323846 / 180.0;
    Mat4 Ry = Mat4::rotateY(yaw);

    // transforma: centraliza -> escala -> cisalha -> gira -> coloca
    Mat4 Tpaper =
        Mat4::translation(papCX, papCY, papCZ) *
        Ry *
        Shear *
        Mat4::scale(papL, papH, papW) *
        Mat4::translation(-0.5, -0.5, -0.5);

    {
        auto papeis = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KePaper, KdPaper, KaPaper, mPaper
        );

        papeis->setTransform(Tpaper);
        objetos.push_back(std::move(papeis));
    }

    //  PATINHO (malha) + BICO (cone separado) 
    const double PI = 3.14159265358979323846;

    // ajuste posição em cima da mesa
    double tableY = altura + tampoEsp;

    // escala (um pouco menor do que antes)
    double duckS = 0.28;

    // limites aproximados do patinho.obj (do arquivo que eu te mandei)
    Vec4 duckMin{0.0, 0.0, 0.0, 1};
    Vec4 duckMax{3.70, 1.70, 1.20, 1};
    Vec4 duckCenterLocal{
        (duckMin.x + duckMax.x)*0.5,
        (duckMin.y + duckMax.y)*0.5,
        (duckMin.z + duckMax.z)*0.5,
        1
    };

    double duckHLocal = duckMax.y - duckMin.y;

    // posição do centro do pato no mundo (em cima da mesa)
    double duckCX = x0 + 6.5;
    double duckCZ = z0 + 4.8;
    double duckCY = tableY + 0.02 + (duckHLocal * duckS)*0.5;

    // rotação: +X -> -Z (fica de frente pro eixo Z negativo / câmera)
    Mat4 Rduck = Mat4::rotateY(+PI/2.0);

    // matriz “base” do pato (centra -> escala -> gira -> coloca no mundo)
    Mat4 Mduck =
        Mat4::translation(duckCX, duckCY, duckCZ) *
        Rduck *
        Mat4::scale(duckS, duckS, duckS) *
        Mat4::translation(-duckCenterLocal.x, -duckCenterLocal.y, -duckCenterLocal.z);

    // material do pato (amarelo borracha)
    Vec4 KeDuck{0.08,0.08,0.08,0};
    Vec4 KdDuck{0.95,0.85,0.15,0};
    Vec4 KaDuck{0.25,0.22,0.06,0};
    double mDuck = 25.0;

    // 1) Malha do pato
    {
        auto patinho = std::make_unique<Malha>(
            "models/patinho.obj",
            KeDuck, KdDuck, KaDuck, mDuck
        );

        patinho->setTransform(Mduck);
        objetos.push_back(std::move(patinho));
    }

    // 2) Bico laranja (cone separado)
    // Base do bico no OBJ original (aprox): x=3.10, y~1.20, z~0.60
    Vec4 beakBaseLocal{3.10, 1.20, 0.60, 1};

    // dimensões do bico em unidades LOCAIS do OBJ (antes da escala duckS)
    double beakLen = 0.70;   // comprimento
    double beakR   = 0.20;   // raio da base

    // material laranja
    Vec4 KeBeak{0.05,0.05,0.05,0};
    Vec4 KdBeak{1.00,0.45,0.10,0};
    Vec4 KaBeak{0.25,0.12,0.04,0};
    double mBeak = 15.0;

    // cone nasce no +Y -> queremos que ele aponte no +X local do pato,
    // e depois a rotação do pato leva +X -> -Z (frente da câmera)
    Mat4 RbeakLocal = Mat4::rotateZ(-PI/2.0);

    {
        auto bico = std::make_unique<Cone>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeBeak, KdBeak, KaBeak, mBeak
        );

        Mat4 Mbeak =
            // mesma base do pato (centra/escala/roda/coloca)
            Mat4::translation(duckCX, duckCY, duckCZ) *
            Rduck *
            Mat4::scale(duckS, duckS, duckS) *
            Mat4::translation(-duckCenterLocal.x, -duckCenterLocal.y, -duckCenterLocal.z) *
            // posiciona no local do bico
            Mat4::translation(beakBaseLocal.x, beakBaseLocal.y, beakBaseLocal.z) *
            RbeakLocal *
            Mat4::scale(beakR, beakLen, beakR);

        bico->setTransform(Mbeak);
        objetos.push_back(std::move(bico));
    }


    // ---------------- MOVA GLOBE (simples) ----------------
    // Material metal (bem diferente da madeira e do notebook)
    Vec4 KeMetal{0.60, 0.60, 0.60, 0};
    Vec4 KdMetal{0.35, 0.35, 0.35, 0};
    Vec4 KaMetal{0.05, 0.05, 0.05, 0};
    double mMetal = 140.0;
    
    // Posição do conjunto em cima da mesa
    double tableTopY = altura + tampoEsp;     // topo do tampo
    double cx = 2;                          // centro do globo (X)
    double cz = 8;                          // centro do globo (Z)
    
    // Base (disco)
    double baseR = 1.35;
    double baseH = 0.12;
    
    // Hastes (3)
    double rodR   = 0.10;
    double rFoot  = 1.05;                     // raio onde ficam os pés (em torno do centro)
    double angDeg = 60.0;
    double angRad = angDeg * 3.14159265358979323846 / 180.0;
    
    // Define a altura do centro do globo para que a haste fique a 60° da mesa
    double vertical = rFoot * std::tan(angRad);     // tan(60)=1.732...
    double globeCy = (tableTopY + baseH) + vertical;
    
    // Centro do globo
    Vec4 globeCenter(cx, globeCy, cz, 1);
    
    // Comprimento da haste até o centro (de acordo com 60°)
    double rodL = std::sqrt(rFoot*rFoot + vertical*vertical);
    
    // Raio do globo (encaixado entre as hastes)
    double globeR = 0.85;
    
    // Escalar
    double S = 0.4;
    
    baseR *= S;
    baseH *= S;
    rodR  *= S;
    rFoot *= S;
    globeR *= S;
    
    // recomputa com o novo rFoot
    vertical = rFoot * std::tan(angRad);
    rodL = std::sqrt(rFoot*rFoot + vertical*vertical);
    globeCy = (tableTopY + baseH) + vertical;
    
    //  Cilindro base (achatado)
    {
        auto base = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeMetal, KdMetal, KaMetal, mMetal
        );

        // base começa em y=tableTopY e sobe baseH
        base->setTransform(
            Mat4::translation(cx, tableTopY + 0.001, cz) *
            Mat4::scale(baseR, baseH, baseR)
        );
        
        objetos.push_back(std::move(base));
    }
    
    // 3 hastes inclinadas 
    for (int k = 0; k < 3; k++) {
        double phi = (2.0 * 3.14159265358979323846 / 3.0) * k;
        
        // pé da haste sobre o disco (um pouco acima da base)
        Vec4 foot(
            cx + rFoot * std::cos(phi),
            tableTopY + baseH,
            cz + rFoot * std::sin(phi),
            1
        );
        
        // direção desejada: do pé até o centro do globo
        Vec4 vdir = normalize(globeCenter - foot); // w=0
        
        // rotação que leva eixo Y -> vdir (via quatérnio)
        Vec4 yAxis(0,1,0,0);
        double dotv = std::max(-1.0, std::min(1.0, yAxis.dot(vdir)));
        double angle = std::acos(dotv);
        
        Vec4 axis = yAxis.cross(vdir); // eixo de rotação
        Mat4 R = Mat4::identity();
        
        if (axis.length() > 1e-9) {
            RTQuat q = RTQuat::fromAxisAngle(axis, angle);
            R = q.toMat4();
        } else {
            // paralelo (mesma direção ou oposta)
            if (dotv < 0.0) {
                RTQuat q = RTQuat::fromAxisAngle(Vec4(1,0,0,0), 3.14159265358979323846);
                R = q.toMat4();
            }
        }
        
        auto haste = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 0.6,
            KeMetal, KdMetal, KaMetal, mMetal
        );
        
        // base no foot, apontando para o centro, com comprimento rodL
        haste->setTransform(
            Mat4::translation(foot.x, foot.y, foot.z) *
            R *
            Mat4::scale(rodR, rodL, rodR)
        );
        
        objetos.push_back(std::move(haste));
    }
    
    //  Globo (esfera) 
    {
        auto globo = std::make_unique<Esfera>(
            Vec4(0,0,0,1), globeR,
            Vec4(0.10,0.10,0.10,0),  // Ke (spec) do globo
            Vec4(0.7,0.7,0.7,0),  // Kd (vai ser modulada pela textura)
            Vec4(0.2,0.2,0.2,0),  // Ka
            80.0
        );
        
        // textura 
        auto texGlobo =std::make_shared<ImageTexturePPM>("models/flat_earth03.ppm");
        globo->setTexture(texGlobo);
        
        globo->setTransform(Mat4::translation(globeCenter.x, globeCenter.y-1.21, globeCenter.z));
        objetos.push_back(std::move(globo));
    }
        

    // ---------------- ABAJUR ----------------
    Vec4 KeMetalLamp{0.50, 0.50, 0.50, 0};
    Vec4 KdMetalLamp{0.25, 0.25, 0.25, 0};
    Vec4 KaMetalLamp{0.04, 0.04, 0.04, 0};
    double mMetalLamp = 120.0;

    Vec4 KeCupula{0.08, 0.08, 0.08, 0};
    Vec4 KdCupula{0.85, 0.78, 0.55, 0};
    Vec4 KaCupula{0.10, 0.09, 0.06, 0};
    double mCupula = 20.0;

    Vec4 KeBulb{0.30, 0.30, 0.30, 0};
    Vec4 KdBulb{1.00, 1.00, 0.95, 0};
    Vec4 KaBulb{0.18, 0.18, 0.17, 0};
    double mBulb = 60.0;

    double lampX = x0 + mesaX - 1.0;
    double lampZ = z0 + 5.0;
    double tableTopYAbajur = altura + tampoEsp;

    double baseRAbajur = 0.75;
    double baseHAbajur = 0.10;
    double stemR = 0.07;
    double stemH = 1.25;
    double shadeR = 1.10;
    double shadeH = 1.05;

    double tilt = 0.25;
    Mat4 Rtilt = Mat4::identity();
    {
        RTQuat q = RTQuat::fromAxisAngle(Vec4{0,0,1,0}, -tilt);
        Rtilt = q.toMat4();
    }

    // base
    {
        auto base = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeMetalLamp, KdMetalLamp, KaMetalLamp, mMetalLamp
        );
        base->setTransform(
            Mat4::translation(lampX, tableTopYAbajur + 0.001, lampZ) *
            Mat4::scale(baseRAbajur, baseHAbajur, baseRAbajur)   // << CORRIGIDO (baseHAbajur)
        );
        objetos.push_back(std::move(base));
    }

    // haste
    {
        auto haste = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeMetalLamp, KdMetalLamp, KaMetalLamp, mMetalLamp
        );
        haste->setTransform(
            Mat4::translation(lampX, tableTopYAbajur + baseHAbajur, lampZ) *
            Rtilt *
            Mat4::scale(stemR, stemH, stemR)
        );
        objetos.push_back(std::move(haste));
    }

    // cúpula
    {
        auto cupula = std::make_unique<Cone>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeCupula, KdCupula, KaCupula, mCupula
        );
        double cupulaBaseY = tableTopYAbajur + baseHAbajur + stemH - 0.35;
        cupula->setTransform(
            Mat4::translation(lampX, cupulaBaseY, lampZ) *
            Rtilt *
            Mat4::scale(shadeR, shadeH, shadeR)
        );
        objetos.push_back(std::move(cupula));
    }

    // lâmpada
    {
        auto lampada = std::make_unique<Esfera>(
            Vec4(0,0,0,1), 0.20,
            KeBulb, KdBulb, KaBulb, mBulb
        );
        double bulbY = tableTopYAbajur + baseHAbajur + stemH - 0.15;
        lampada->setTransform(Mat4::translation(lampX, bulbY, lampZ));
        objetos.push_back(std::move(lampada));
    }

    // notebook
    {
        auto notebook = std::make_unique<Malha>(
            "models/notebook.obj",
            Vec4{0.25,0.25,0.25,0},
            Vec4{0.15,0.15,0.15,0},
            Vec4{0.06,0.06,0.06,0},
            40.0
        );
        notebook->setTransform(
            Mat4::translation(4.0, altura + tampoEsp + 0.01, 3.75) *
            Mat4::scale(0.65, 1.0, 0.65)
        );
        objetos.push_back(std::move(notebook));
    }

    // ---------------- RENDER ----------------
    for (int l = 0; l < height; ++l) {
        for (int c = 0; c < width; ++c) {
            Vec4 origem, rayDir;
            cam.geraRaio(c, l, width, height, origem, rayDir);

            double tMin = 1e18;
            Objeto* objHit = nullptr;
            Vec4 pHit;
            Colisao tipoHit = Colisao::Nenhuma;

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

            Color corFinal(30,30,30); // fundo

            if (objHit) {
                bool isInShadow = false;
                Vec4 P = pHit;

                if (luzPontual.factorAt(P) <= 0.0) {
                    isInShadow = true;
                } else {
                    Vec4 shadowDir = luzPontual.Lvec(P);
                    double distLimit = luzPontual.maxDistance(P);

                    Vec4 shadowOrigin = P + shadowDir * 1e-3;

                    for (auto& obj : objetos) {
                        if (obj.get() == objHit) continue;

                        Vec4 pS;
                        double tS;
                        Colisao tipoS;
                        if (obj->intersect(shadowOrigin, shadowDir, pS, tS, tipoS)) {
                            if (tS > 1e-6 && tS < distLimit - 1e-4) {
                                isInShadow = true;
                                break;
                            }
                        }
                    }
                }

                corFinal = objHit->calculaCor(origem, pHit, luzPontual, luzAmb, tipoHit, isInShadow);
            }

            saveColor(img, corFinal);
        }
        img << "\n";
    }

    img.close();
    std::cout << "Imagem gerada com sucesso (saida.ppm).\n";
    return 0;
}
