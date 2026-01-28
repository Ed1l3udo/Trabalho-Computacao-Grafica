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
// #include "Texture.hpp"

// ------------------- Config -------------------
static const int PREVIEW_W = 240;   // preview rápido
static const int PREVIEW_H = 240;
static const int SCALE     = 3;     // janela = PREVIEW * SCALE
static const int ROWS_PER_FRAME = 16; // render progressivo

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

// Renderiza uma linha y (0..h-1)
static void renderRow(
    int y, int w, int h,
    const Camera& cam,
    const std::vector<std::unique_ptr<Objeto>>& objetos,
    const Luz& luzPontual,
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
            Vec4 toLight = luzPontual.pos - pHit;
            double distToLight = toLight.length();
            Vec4 shadowDir = normalize(toLight);
            Vec4 shadowOrigin = pHit + shadowDir * 1e-3;

            for (auto& obj : objetos) {
                if (obj.get() == objHit) continue; // evita auto-sombra

                Vec4 pS;
                double tS;
                Colisao tipoS;
                if (obj->intersect(shadowOrigin, shadowDir, pS, tS, tipoS)) {
                    if (tS > 1e-6 && tS < distToLight - 1e-3) {
                        isInShadow = true;
                        break;
                    }
                }
            }

            corFinal = objHit->calculaCor(origem, pHit, luzPontual, luzAmb, tipoHit, isInShadow);

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

        // rotaciona o forward (usando sua Mat4 rotateAxisAngle seria mais elegante,
        // mas aqui fica simples: duas rotações sequenciais com Rodrigues)
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

    Luz luzPontual(Vec4{10, 10, 2, 1}, Vec4{1,1,1,0});
    Luz luzAmb(Vec4{0.2,0.2,0.2,0});

    std::vector<std::unique_ptr<Objeto>> objetos;
    // TODO: adicione seus objetos aqui (esfera/cilindro/cone/caixa/malha etc.)

    auto esfera = std::make_unique<Esfera>(
        Vec4(0, 0, 0, 1), 1,
        Vec4(0.5, 0.6, 0, 0), Vec4(1, 0.8, 0.7, 0), Vec4(0.9, 0.3, 0.4, 0), 50.0);
    esfera->setTransform(Mat4::translation(6, 2, 8)); // coloca no mundo (primeiro octante)

    // auto texGlobo = std::make_shared<CheckerTexture>(
    //     Vec4{0.1, 0.4, 1.0, 0},   // azul
    //     Vec4{0.9, 0.9, 0.9, 0},   // branco
    //     24, 12                   // quantidade de quadrados
    // );

    // esfera->setTexture(texGlobo);

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


    // ------------------- Raylib -------------------
    InitWindow(PREVIEW_W * SCALE, PREVIEW_H * SCALE, "Ray Casting Preview + Picking");
    SetExitKey(KEY_NULL);  // desativa ESC como tecla de fechar
    SetTargetFPS(60);
    DisableCursor(); // opcional: remove cursor para mouse-look (re-habilite com ESC)

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
                selected = pickAt(mx, my, PREVIEW_W, PREVIEW_H, cam, objetos);
                needRestart = true; // refaz para destacar
            }
        }

        // reinicia render
        if (needRestart) {
            // std::fill(rgba.begin(), rgba.end(), 0);
            nextRow = 0;
            needRestart = false;
        }

        // renderiza algumas linhas por frame
        for (int k=0; k<ROWS_PER_FRAME && nextRow < PREVIEW_H; k++, nextRow++) {
            renderRow(nextRow, PREVIEW_W, PREVIEW_H, cam, objetos, luzPontual, luzAmb, selected, rgba);
        }

        // atualiza textura
        UpdateTexture(tex, rgba.data());

        BeginDrawing();
        ClearBackground((RL_Color){0, 0, 0, 255});

        DrawTextureEx(tex, (Vector2){0.0f, 0.0f}, 0.0f, (float)SCALE, (RL_Color){255,255,255,255});

        DrawText("WASD: move | RMB: olhar | Wheel: zoom | Click: pick | ESC: cursor",
                10, 10, 18, (RL_Color){255,255,0,255});
        EndDrawing();

    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
