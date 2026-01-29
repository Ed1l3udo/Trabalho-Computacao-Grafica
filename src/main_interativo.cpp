#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <iostream>
#include <algorithm>

#define Color RL_Color
#define Camera RL_Camera
#include "raylib.h"
#undef Color
#undef Camera
#ifdef PI
#undef PI
#endif

#include "Camera.hpp"
#include "Objeto.hpp"
#include "Luz.hpp"
#include "Colisao.hpp"
#include "Vec4.hpp"
#include "Color.hpp"
#include "Caixa.hpp"
#include "Esfera.hpp"
#include "Cilindro.hpp"
#include "Cone.hpp"
#include "Malha.hpp"
#include "Mat4.hpp"
#include "Texture.hpp"
#include "Quaternion.hpp"

// ------------------- Config -------------------
static const int PREVIEW_W = 240;   // preview rápido
static const int PREVIEW_H = 240;
static const int SCALE     = 3;     // janela = PREVIEW * SCALE
static const int ROWS_PER_FRAME = 16; // render progressivo

static void setPreset1Point(Camera& cam, const Vec4& target, double dist=12.0) {
    cam.proj = Projecao::Perspectiva;
    cam.at = target;
    cam.up = Vec4{0,1,0,0};

    // 1 ponto: alinhado com Z (sem yaw e sem pitch)
    cam.eye = Vec4{ target.x, target.y, target.z - dist, 1 };
}

static void setPreset2Point(Camera& cam, const Vec4& target, double dist=12.0) {
    cam.proj = Projecao::Perspectiva;
    cam.at = target;
    cam.up = Vec4{0,1,0,0};

    // 2 pontos: yaw (muda X e Z), mas sem pitch (eye.y == at.y)
    cam.eye = Vec4{ target.x + dist, target.y, target.z - dist, 1 };
}

static void setPreset3Point(Camera& cam, const Vec4& target, double dist=12.0) {
    cam.proj = Projecao::Perspectiva;
    cam.at = target;
    cam.up = Vec4{0,1,0,0};

    // 3 pontos: yaw + pitch (eye.y != at.y)
    cam.eye = Vec4{ target.x + dist, target.y + dist*0.8, target.z - dist, 1 };
}

// Converte seu Color para RGBA
static inline void putPixelRGBA(std::vector<unsigned char>& rgba, int x, int y, int w, const Color& c) {
    int idx = 4 * (y * w + x);
    rgba[idx+0] = (unsigned char)c.r;
    rgba[idx+1] = (unsigned char)c.g;
    rgba[idx+2] = (unsigned char)c.b;
    rgba[idx+3] = 255;
}

static inline Color blendHighlight(const Color& c) {
    // mistura com amarelo para destacar seleção
    int r = std::min(255, (int)(0.7*c.r + 0.3*255));
    int g = std::min(255, (int)(0.7*c.g + 0.3*255));
    int b = std::min(255, (int)(0.7*c.b + 0.3*0));
    return Color(r,g,b);
}

static std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> lines;
    std::string cur;
    for (char ch : s) {
        if (ch == '\n') { lines.push_back(cur); cur.clear(); }
        else cur.push_back(ch);
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

static std::string fmtVec4(const Vec4& v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return oss.str();
}

static std::string describeObject(const Objeto* obj) {
    if (!obj) return "Nenhum objeto selecionado.";

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    // Posição aproximada do objeto: origem local (0,0,0) no mundo
    Vec4 posW = obj->toWorldPoint(Vec4(0,0,0,1));

    // Materiais e params por tipo
    if (auto e = dynamic_cast<const Esfera*>(obj)) {
        Vec4 cW = obj->toWorldPoint(e->centro);
        oss << "Tipo: Esfera\n";
        oss << "Centro(world): " << fmtVec4(cW) << "\n";
        oss << "Raio: " << e->raio << "\n";
        oss << "Textura: " << (e->tex ? "Sim" : "Nao") << "\n";
        oss << "Ke(spec): " << fmtVec4(e->Ke) << "\n";
        oss << "Kd(diff): " << fmtVec4(e->Kd) << "\n";
        oss << "Ka(amb):  " << fmtVec4(e->Ka) << "\n";
        oss << "m: " << e->m << "\n";
        return oss.str();
    }
    if (auto c = dynamic_cast<const Cilindro*>(obj)) {
        Vec4 baseW = obj->toWorldPoint(c->centroBase);
        oss << "Tipo: Cilindro\n";
        oss << "Base(world): " << fmtVec4(baseW) << "\n";
        oss << "Raio: " << c->raioBase << "\n";
        oss << "Altura: " << c->altura << "\n";
        oss << "Ke(spec): " << fmtVec4(c->Ke) << "\n";
        oss << "Kd(diff): " << fmtVec4(c->Kd) << "\n";
        oss << "Ka(amb):  " << fmtVec4(c->Ka) << "\n";
        oss << "m: " << c->m << "\n";
        return oss.str();
    }
    if (auto c = dynamic_cast<const Cone*>(obj)) {
        Vec4 baseW = obj->toWorldPoint(c->centroBase);
        oss << "Tipo: Cone\n";
        oss << "Base(world): " << fmtVec4(baseW) << "\n";
        oss << "Raio: " << c->raioBase << "\n";
        oss << "Altura: " << c->altura << "\n";
        oss << "Ke(spec): " << fmtVec4(c->Ke) << "\n";
        oss << "Kd(diff): " << fmtVec4(c->Kd) << "\n";
        oss << "Ka(amb):  " << fmtVec4(c->Ka) << "\n";
        oss << "m: " << c->m << "\n";
        return oss.str();
    }
    if (auto b = dynamic_cast<const Caixa*>(obj)) {
        Vec4 minW = obj->toWorldPoint(b->bmin);
        Vec4 maxW = obj->toWorldPoint(b->bmax);
        oss << "Tipo: Caixa (AABB local)\n";
        oss << "bmin(local)->world: " << fmtVec4(minW) << "\n";
        oss << "bmax(local)->world: " << fmtVec4(maxW) << "\n";
        oss << "Ke(spec): " << fmtVec4(b->Ke) << "\n";
        oss << "Kd(diff): " << fmtVec4(b->Kd) << "\n";
        oss << "Ka(amb):  " << fmtVec4(b->Ka) << "\n";
        oss << "m: " << b->m << "\n";
        return oss.str();
    }
    if (auto m = dynamic_cast<const Malha*>(obj)) {
        oss << "Tipo: Malha\n";
        oss << "Pos(world aprox): " << fmtVec4(posW) << "\n";
        oss << "Vertices: " << m->vertices.size() << "\n";
        oss << "Triangulos: " << m->tris.size() << "\n";
        oss << "Ke(spec): " << fmtVec4(m->Ke) << "\n";
        oss << "Kd(diff): " << fmtVec4(m->Kd) << "\n";
        oss << "Ka(amb):  " << fmtVec4(m->Ka) << "\n";
        oss << "m: " << m->m << "\n";
        return oss.str();
    }

    // fallback
    oss << "Tipo: Objeto (desconhecido)\n";
    oss << "Pos(world): " << fmtVec4(posW) << "\n";
    return oss.str();
}

static void drawInfoPanel(const std::string& info, int x, int y) {
    auto lines = splitLines(info);
    int fontSize = 18;
    int lineH = fontSize + 4;

    int w = 0;
    for (auto& s : lines) w = std::max(w, MeasureText(s.c_str(), fontSize));
    int h = (int)lines.size() * lineH + 10;

    // fundo semi-transparente
    DrawRectangle(x-6, y-6, w+12, h+12, (RL_Color){0,0,0,160});
    DrawRectangleLines(x-6, y-6, w+12, h+12, (RL_Color){255,255,0,200});

    for (int i=0;i<(int)lines.size();i++) {
        DrawText(lines[i].c_str(), x, y + i*lineH, fontSize, (RL_Color){255,255,255,255});
    }
}


// Renderiza uma linha y (0..h-1)
static void renderRow(
    int y, int w, int h,
    const Camera& cam,
    const std::vector<std::unique_ptr<Objeto>>& objetos,
    const Luz& luzPrincipal,
    const Luz& luzAmb,
    Objeto* selected,
    std::vector<unsigned char>& outRGBA
){
    for (int x=0; x<w; x++) {
        Vec4 origem, rayDir;
        cam.geraRaio(x, y, w, h, origem, rayDir);

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
            // sombra (shadow ray)
            bool isInShadow = false;
            if (luzPrincipal.factorAt(pHit) <= 0.0) {
                isInShadow = true;
            } else {
                Vec4 shadowDir = luzPrincipal.Lvec(pHit);
                double distLimit = luzPrincipal.maxDistance(pHit);
                Vec4 shadowOrigin = pHit + shadowDir * 1e-3;

                for (auto& obj : objetos) {
                    if (obj.get() == objHit) continue; // evita auto-sombra

                    Vec4 pS;
                    double tS;
                    Colisao tipoS;
                    if (obj->intersect(shadowOrigin, shadowDir, pS, tS, tipoS)) {
                        if (tS > 1e-6 && tS < distLimit - 1e-3) {
                            isInShadow = true;
                            break;
                        }
                    }
                }
            }

            corFinal = objHit->calculaCor(origem, pHit, luzPrincipal, luzAmb, tipoHit, isInShadow);

            if (objHit == selected) {
                corFinal = blendHighlight(corFinal);
            }
        }

        putPixelRGBA(outRGBA, x, y, w, corFinal);
    }
}

// Picking: retorna o objeto mais próximo para um pixel (mx,my) na imagem
static Objeto* pickAt(
    int mx, int my, int w, int h,
    const Camera& cam,
    const std::vector<std::unique_ptr<Objeto>>& objetos
){
    Vec4 origem, rayDir;
    cam.geraRaio(mx, my, w, h, origem, rayDir);

    double tMin = 1e18;
    Objeto* objHit = nullptr;

    for (auto& obj : objetos) {
        Vec4 p;
        double t;
        Colisao tipo;
        if (obj->intersect(origem, rayDir, p, t, tipo)) {
            if (t > 1e-6 && t < tMin) {
                tMin = t;
                objHit = obj.get();
            }
        }
    }
    return objHit;
}

// Movimento simples de câmera (WASD + QE) e mouse look
static bool updateCameraFreeFly(Camera& cam, float dt) {
    bool changed = false;

    // base da câmera
    Vec4 forward = normalize(cam.at - cam.eye);
    Vec4 right   = normalize(forward.cross(cam.up));
    Vec4 up      = cam.up;

    float speed = (IsKeyDown(KEY_LEFT_SHIFT) ? 6.0f : 3.0f) * dt;

    Vec4 move(0,0,0,0);
    if (IsKeyDown(KEY_W)) move = move + forward * speed;
    if (IsKeyDown(KEY_S)) move = move - forward * speed;
    if (IsKeyDown(KEY_D)) move = move + right * speed;
    if (IsKeyDown(KEY_A)) move = move - right * speed;
    if (IsKeyDown(KEY_E)) move = move + up * speed;
    if (IsKeyDown(KEY_Q)) move = move - up * speed;

    if (move.length() > 0) {
        cam.eye = cam.eye + move;
        cam.at  = cam.at  + move;
        changed = true;
    }

    // Mouse look (segure botão direito)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 d = GetMouseDelta();
        float sens = 0.0025f;

        // yaw em torno do up
        float yaw = -d.x * sens;
        // pitch em torno do right
        float pitch = -d.y * sens;

        auto rotAxis = [](const Vec4& v, const Vec4& axisRaw, double rad){
            Vec4 axis = normalize(axisRaw);
            double c = std::cos(rad), s = std::sin(rad);
            return v*c + axis.cross(v)*s + axis*(axis.dot(v))*(1.0 - c);
        };

        forward = rotAxis(forward, up, yaw);
        right   = normalize(forward.cross(up));
        forward = rotAxis(forward, right, pitch);

        cam.at = cam.eye + forward;
        changed = true;
    }

    // Zoom (muda janela da câmera)
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        double factor = (wheel > 0) ? 0.9 : 1.1; // zoom in/out
        cam.xmin *= factor; cam.xmax *= factor;
        cam.ymin *= factor; cam.ymax *= factor;
        changed = true;
    }

    return changed;
}

int main() {
    // ------------------- Cena: aqui você usa sua cena atual -------------------
    Camera cam;
    cam.eye = Vec4{6, 6, 2, 1};
    cam.at  = Vec4{6, 2, 8, 1};
    cam.up  = Vec4{0, 1, 0, 0};
    cam.d = 1.0;
    cam.xmin = -1.2; cam.xmax = 1.2;
    cam.ymin = -1.2; cam.ymax = 1.2;

    
    std::vector<std::unique_ptr<Objeto>> objetos;

    // CHÃO

    Vec4 KeFloor{0.02,0.02,0.02,0};
    Vec4 KdFloor{0.35,0.30,0.25,0}; 
    Vec4 KaFloor{0.10,0.08,0.06,0};
    double mFloor = 15.0;
    // Dimensões do ambiente 
    double roomX = 14.0;   // tamanho em X
    double roomZ = 12.0;   // tamanho em Z
    double roomH = 6.0;    // altura (Y)

    double wallT = 0.20;   // espessura paredes/teto/chão

    double rx0 = -1;      // origem do quarto
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
    
    //  MESA (tampo + 4 pernas) 
    
    // Materiais
    Vec4 KeMadeira{0.05, 0.05, 0.05, 0};
    Vec4 KdMadeira{0.55, 0.32, 0.15, 0};   // marrom
    Vec4 KaMadeira{0.10, 0.06, 0.03, 0};
    double mMadeira = 10.0;
    
    Vec4 KePreto{0.25, 0.25, 0.25, 0};     // um pouco de spec
    Vec4 KdPreto{0.05, 0.05, 0.05, 0};     // quase preto
    Vec4 KaPreto{0.02, 0.02, 0.02, 0};
    double mPreto = 80.0;
    
    // Dimensões da mesa
    double mesaX = 10.0;     // comprimento em X
    double mesaZ = 6.0;      // largura em Z
    double tampoEsp = 0.25;  // espessura do tampo
    double altura = 2.0;     // altura até o topo do tampo
    
    double pernaEsp = 0.20;  // espessura da perna (quadrada)
    
    // Posição base da mesa
    double x0 = 1.0;
    double z0 = 3.0;
    
    // Tampo: caixa unitária escalada e posicionada
    {
        auto tampo = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KeMadeira, KdMadeira, KaMadeira, mMadeira
        );
        
        // tampo começa em y = altura (base do tampo) e vai até altura + tampoEsp
        Mat4 T = Mat4::translation(x0, altura, z0) * Mat4::scale(mesaX, tampoEsp, mesaZ);
        tampo->setTransform(T);
        objetos.push_back(std::move(tampo));
    }
    
    // Pernas (4): todas pretas, altura = altura (até base do tampo)
    auto criaPerna = [&](double px, double pz) {
        auto perna = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KePreto, KdPreto, KaPreto, mPreto
        );
        
        // Perna vai do chão (y=0) até y=altura
        Mat4 T = Mat4::translation(px, 0.0, pz) * Mat4::scale(pernaEsp, altura, pernaEsp);
        perna->setTransform(T);
        objetos.push_back(std::move(perna));
    };
    
    // Cantos das pernas (um pouco "para dentro" do tampo)
    double inset = 0.35;
    
    double px1 = x0 + inset;
    double px2 = x0 + mesaX - inset - pernaEsp;
    double pz1 = z0 + inset;
    double pz2 = z0 + mesaZ - inset - pernaEsp;
    
    criaPerna(px1, pz1);
    criaPerna(px2, pz1);
    criaPerna(px1, pz2);
    criaPerna(px2, pz2);

    Vec4 sceneTarget{ x0 + mesaX*0.5, altura + tampoEsp + 0.8, z0 + mesaZ*0.5, 1 };

    // LAPIS (cilindro + cone) 

    // Materiais
    Vec4 KePencil{0.15,0.15,0.15,0};
    Vec4 KdPencil{0.95,0.80,0.20,0};   // amarelo lápis
    Vec4 KaPencil{0.10,0.08,0.02,0};
    double mPencil = 40.0;

    Vec4 KeTip{0.10,0.10,0.10,0};
    Vec4 KdTip{0.85,0.65,0.45,0};      // “madeira” / ponta
    Vec4 KaTip{0.08,0.06,0.04,0};
    double mTip = 25.0;

    // Posição do lápis (em cima da mesa)
    double px = 8.5;
    double py = (altura + tampoEsp) + 0.03;  // um pouco acima do tampo
    double pz = 4;

    // Dimensões
    double pencilLen = 1.2;
    double pencilR   = 0.06;

    double tipLen = 0.25;
    double tipR   = 0.07;

    // Rotação para deitar o eixo Y -> eixo Z (90 graus)
    Mat4 Rdeitar = Mat4::rotateX( 3.14159265358979323846/2.0 );
    // Corpo do lápis (cilindro)
    {
        auto corpo = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KePencil, KdPencil, KaPencil, mPencil
        );

        // Cilindro local vai de y=0 até y=1; após scale, vai de 0..pencilLen ao longo do eixo do lápis
        corpo->setTransform(
            Mat4::translation(px, py, pz) *
            Rdeitar *
            Mat4::scale(pencilR, pencilLen, pencilR)
        );

        objetos.push_back(std::move(corpo));
    }

    // Ponta (cone) na extremidade “da frente” do lápis
    {
        auto ponta = std::make_unique<Cone>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeTip, KdTip, KaTip, mTip
        );

        // A base do cone deve encostar no fim do cilindro.
        // Como o cilindro vai de 0..pencilLen no eixo local (Y), a ponta começa em pencilLen.
        ponta->setTransform(
            Mat4::translation(px, py, pz) *
            Rdeitar *
            Mat4::translation(0, pencilLen, 0) *
            Mat4::scale(tipR, tipLen, tipR)
        );

        objetos.push_back(std::move(ponta));
    }

    // BORRACHA (caixinha branca) + ESPELHO em x = centro da mesa 

    // plano de espelho x = centro da mesa
    double xCenterMesa = x0 + mesaX * 0.5;
    Mat4 espelhoMesaX = Mat4::mirrorPlane(Vec4(xCenterMesa, 0, 0, 1), Vec4(1, 0, 0, 0));

    // material branco (bem simples)
    Vec4 KeE{0.05,0.05,0.05,0};
    Vec4 KdE{0.95,0.95,0.95,0};
    Vec4 KaE{0.25,0.25,0.25,0};
    double mE = 20.0;

    double yawDeg = 110.0; // +Z com leve -X (esquerda)
    double yawRad = yawDeg * 3.14159265358979323846 / 180.0;
    Mat4 Rer = Mat4::rotateY(yawRad);

    // posição base da borracha (em cima do tampo)
    double erX = x0 + 7.2;                         // ajuste se quiser
    double erY = altura + tampoEsp + 0.02;         // bem em cima do tampo
    double erZ = z0 + 1.2;                         // ajuste se quiser

    // dimensões (em "metros" do seu mundo)
    double erL = 0.30;   // comprimento (eixo local X)
    double erH = 0.12;   // altura
    double erW = 0.18;   // largura (eixo local Z)

    // centro da borracha em cima do tampo
    double erCX = x0 + 8.2;                     
    double erCY = (altura + tampoEsp) + 0.02 + erH*0.5;
    double erCZ = z0 + 1.6;

    // transforma unit cube (0..1) -> centra no 0 -> escala -> rotaciona -> coloca no centro
    Mat4 Ter =
        Mat4::translation(erCX, erCY, erCZ) *
        Rer *
        Mat4::scale(erL, erH, erW) *
        Mat4::translation(-0.5, -0.5, -0.5);
        
    // Borracha 1 (original)
    {
        auto borracha1 = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KeE, KdE, KaE, mE
        );
        borracha1->setTransform(Ter);
        objetos.push_back(std::move(borracha1));
    }

    // Borracha 2 (espelhada no plano x = centro da mesa)
    {
        auto borracha2 = std::make_unique<Caixa>(
            Vec4(0,0,0,1), Vec4(1,1,1,1),
            KeE, KdE, KaE, mE
        );

        // aplica espelho ao transform original
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

    
    // MOVA GLOBE (4 cilindros + esfera) 
    
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

    //  ABAJUR (base + haste + cupula + lampada)
    
    // Materiais (bem distintos)
    Vec4 KeMetalLamp{0.50, 0.50, 0.50, 0};
    Vec4 KdMetalLamp{0.25, 0.25, 0.25, 0};
    Vec4 KaMetalLamp{0.04, 0.04, 0.04, 0};
    double mMetalLamp = 120.0;
    
    Vec4 KeCupula{0.08, 0.08, 0.08, 0};
    Vec4 KdCupula{0.85, 0.78, 0.55, 0};   // “tecido” bege
    Vec4 KaCupula{0.10, 0.09, 0.06, 0};
    double mCupula = 20.0;
    
    Vec4 KeBulb{0.30, 0.30, 0.30, 0};
    Vec4 KdBulb{1.00, 1.00, 0.95, 0};
    Vec4 KaBulb{0.18, 0.18, 0.17, 0};
    double mBulb = 60.0;
    
    // Posição do abajur em cima da mesa
    double tableTopYAbajur = altura + tampoEsp;
    double lampX = x0 + mesaX - 1.0;  // canto direito do tampo
    double lampZ = z0 + 5;          // perto da borda frontal
    
    // Dimensões
    double baseRAbajur = 0.75;
    double baseHAbajur = 0.10;
    
    double stemR = 0.07;
    double stemH = 1.25;
    
    double shadeR = 1.10;
    double shadeH = 1.05;
    
    // Inclinação do abajur (via quatérnio)
    double tilt = 0.25; // rad (~14°)
    Mat4 Rtilt = Mat4::identity();
    {
        // inclina em torno de um eixo “diagonal”
        RTQuat q = RTQuat::fromAxisAngle(Vec4{0,0,1,0}, -tilt); // inclina para frente
        Rtilt = q.toMat4();
    }
    
    // 1) Base (disco)
    {
        auto base = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeMetalLamp, KdMetalLamp, KaMetalLamp, mMetalLamp
        );
        
        base->setTransform(
            Mat4::translation(lampX, tableTopYAbajur + 0.001, lampZ) *
            Mat4::scale(baseRAbajur, baseH, baseRAbajur)
        );
        
        objetos.push_back(std::move(base));
    }

    // 2) Haste (cilindro fino)
    {
        auto haste = std::make_unique<Cilindro>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeMetalLamp, KdMetalLamp, KaMetalLamp, mMetalLamp
        );
        
        // base da haste em cima da base; aplica inclinação
        haste->setTransform(
            Mat4::translation(lampX, tableTopYAbajur + baseHAbajur, lampZ) *
            Rtilt *
            Mat4::scale(stemR, stemH, stemR)
        );
        
        objetos.push_back(std::move(haste));
    }
    
    // 3) Cúpula (cone)
    {
        auto cupula = std::make_unique<Cone>(
            Vec4(0,0,0,1), Vec4(0,1,0,0), 1, 1,
            KeCupula, KdCupula, KaCupula, mCupula
        );
        
        double cupulaBaseY = tableTopYAbajur + baseH + stemH - 0.35;
        
        cupula->setTransform(
            Mat4::translation(lampX, cupulaBaseY, lampZ) *
            Rtilt *
            Mat4::scale(shadeR, shadeH, shadeR)
        );
        
        objetos.push_back(std::move(cupula));
    }
    
    // 4) Lâmpada (esfera pequena)
    {
        auto lampada = std::make_unique<Esfera>(
            Vec4(0,0,0,1), 0.20,
            KeBulb, KdBulb, KaBulb, mBulb
        );
        
        // coloca “dentro” da cúpula (aproximado)
        double bulbY = tableTopYAbajur + baseHAbajur + stemH - 0.15;
        
        lampada->setTransform(
            Mat4::translation(lampX, bulbY, lampZ)
        );
        
        objetos.push_back(std::move(lampada));
    }
    
    
    // notebook
    auto notebook = std::make_unique<Malha>(
        "models/notebook.obj",
        Vec4{0.25,0.25,0.25,0},   // Ke (spec)
        Vec4{0.15,0.15,0.15,0},   // Kd (difuso)
        Vec4{0.06,0.06,0.06,0},   // Ka (ambiente)
        40.0
    );
    
    // colocar em cima da mesa (tudo positivo)
    notebook->setTransform(Mat4::translation(4.0, altura + tampoEsp + 0.01, 3.75) *Mat4::scale(0.65, 1.0, 0.65));
    
    objetos.push_back(std::move(notebook));
    
        Luz luzPontual(Vec4{10, 10, 2, 1}, Vec4{1,1,1,0});
        Luz luzAmb(Vec4{0.2,0.2,0.2,0});
        Luz luzDir  = Luz::Direcional(Vec4{-1,-1,0,0}, Vec4{1,1,1,0});
        Luz luzSpot = Luz::Spot(
            Vec4{lampX, tableTopY +0.90, lampZ, 1},   // posição da lâmpada
            Vec4{0, -1, 0, 0},                                         // aponta para baixo
            85.0,                                                      // cutoff graus
            Vec4{1, 1, 1, 0}
        );
    
    
        Luz luzPrincipal = luzPontual;
        
        // ------------------- Raylib -------------------
        InitWindow(PREVIEW_W * SCALE, PREVIEW_H * SCALE, "Ray Casting Preview + Picking");
        SetExitKey(KEY_NULL);  // desativa ESC como tecla de fechar
        SetTargetFPS(60);
        DisableCursor(); 
        
        // buffer RGBA
        std::vector<unsigned char> rgba(PREVIEW_W * PREVIEW_H * 4, 0);
        Image img = {
            .data = rgba.data(),
            .width = PREVIEW_W,
            .height = PREVIEW_H,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    Texture2D tex = LoadTextureFromImage(img);

    Objeto* selected = nullptr;
    std::string selectedInfo = "Nenhum objeto selecionado.";

    // render progressivo
    int nextRow = 0;
    bool needRestart = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ESC para liberar cursor
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (IsCursorHidden()) EnableCursor();
            else DisableCursor();
        }

        bool camChanged = updateCameraFreeFly(cam, dt);
        if (camChanged) {
            needRestart = true;
        }

        // picking com clique esquerdo
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mp = GetMousePosition();
            int mx = (int)(mp.x / SCALE);
            int my = (int)(mp.y / SCALE);

            if (mx >=0 && mx < PREVIEW_W && my >=0 && my < PREVIEW_H) {
                Objeto* hit = pickAt(mx, my, PREVIEW_W, PREVIEW_H, cam, objetos);

                // Toggle: clicou no mesmo -> desmarca; clicou no vazio -> desmarca
                if (hit == nullptr || hit == selected) selected = nullptr;
                else selected = hit;

                selectedInfo = describeObject(selected);
                needRestart = true;
            }
        }

        // reinicia render
        if (needRestart) {
            // std::fill(rgba.begin(), rgba.end(), 0);
            nextRow = 0;
            needRestart = false;
        }

        // Troca de projeção
        if (IsKeyPressed(KEY_ONE))  { cam.proj = Projecao::Perspectiva;  needRestart = true; }
        if (IsKeyPressed(KEY_TWO))  { cam.proj = Projecao::Ortografica; needRestart = true; }
        if (IsKeyPressed(KEY_THREE)){ cam.proj = Projecao::Obliqua;     needRestart = true; }

        // Ajustes da oblíqua (opcional, mas ótimo para demonstrar)
        // Z = cabinet (0.5), X = cavalier (1.0)
        if (IsKeyPressed(KEY_Z)) { cam.obliqL = 0.5; needRestart = true; }
        if (IsKeyPressed(KEY_X)) { cam.obliqL = 1.0; needRestart = true; }

        // Pontos de fuga
        if (IsKeyPressed(KEY_F1)) { setPreset1Point(cam, sceneTarget); needRestart = true; }
        if (IsKeyPressed(KEY_F2)) { setPreset2Point(cam, sceneTarget); needRestart = true; }
        if (IsKeyPressed(KEY_F3)) { setPreset3Point(cam, sceneTarget); needRestart = true; }

        if (IsKeyPressed(KEY_L)) {
            if (luzPrincipal.tipo == TipoLuz::Pontual) luzPrincipal = luzDir;
            else if (luzPrincipal.tipo == TipoLuz::Direcional) luzPrincipal = luzSpot;
            else luzPrincipal = luzPontual;
            needRestart = true;
        }

        if (IsKeyPressed(KEY_C)) { selected = nullptr; selectedInfo = describeObject(nullptr); needRestart = true; }

        // ajustar cutoff do spot
        if (luzPrincipal.tipo == TipoLuz::Spot) {
            static double cutoffDeg = 20.0;
            if (IsKeyPressed(KEY_LEFT_BRACKET)) { cutoffDeg = std::max(5.0, cutoffDeg - 5.0); luzPrincipal.cutoffCos = std::cos(cutoffDeg * Luz::kPi / 180.0); needRestart = true; }
            if (IsKeyPressed(KEY_RIGHT_BRACKET)) { cutoffDeg = std::min(60.0, cutoffDeg + 5.0); luzPrincipal.cutoffCos = std::cos(cutoffDeg * Luz::kPi / 180.0); needRestart = true; }
        }


        // renderiza algumas linhas por frame
        for (int k=0; k<ROWS_PER_FRAME && nextRow < PREVIEW_H; k++, nextRow++) {
            renderRow(nextRow, PREVIEW_W, PREVIEW_H, cam, objetos, luzPrincipal, luzAmb, selected, rgba);
        }

        // atualiza textura
        UpdateTexture(tex, rgba.data());

        BeginDrawing();
        ClearBackground((RL_Color){0, 0, 0, 255});

        DrawTextureEx(tex, (Vector2){0.0f, 0.0f}, 0.0f, (float)SCALE, (RL_Color){255,255,255,255});

        DrawText("WASD move | RMB look | Wheel zoom | Click pick | ESC cursor", 10, 10, 18, (RL_Color){255,255,0,255});
        DrawText("1 Persp | 2 Ortho | 3 Oblique | Z Cabinet | X Cavalier", 10, 32, 18, (RL_Color){255,255,0,255});
        DrawText("F1 1-point | F2 2-point | F3 3-point", 10, 54, 18, (RL_Color){255,255,0,255});
        DrawText("L: Change Light", 10, 76, 18, (RL_Color){255,255,0,255});

        drawInfoPanel(selectedInfo, 10, 120);
        DrawText("Clique no mesmo objeto para cancelar selecao | C: limpar", 10, 96, 18, (RL_Color){255,255,0,255});

        EndDrawing();

    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
