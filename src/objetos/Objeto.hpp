#ifndef OBJETO_HPP
#define OBJETO_HPP

#include "Vec4.hpp"
#include "Color.hpp"
#include "Luz.hpp"
#include "Colisao.hpp"
#include "Transform.hpp"
#include "Mat4.hpp"

class Objeto {
protected:
    Mat4 M    = Mat4::identity(); // objeto -> mundo
    Mat4 Minv = Mat4::identity(); // mundo -> objeto
    Mat4 MinvT = Mat4::identity(); // (Minv)^T para normais

public:
    virtual ~Objeto() = default;

    void setTransform(const Mat4& m) {
        M = m;
        Minv = m.inverse();
        MinvT = Minv.transpose();
    }

    Vec4 toLocalPoint(const Vec4& pW) const { return Minv.mulPoint(pW); }
    Vec4 toLocalVector(const Vec4& vW) const { return Minv.mulVector(vW); }
    Vec4 toWorldPoint(const Vec4& pL) const { return M.mulPoint(pL); }
    Vec4 normalToWorld(const Vec4& nL) const { return normalize(MinvT.mulVector(nL)); }

    // Agora cada objeto implementa APENAS o intersectLocal (no espaço do objeto)
    virtual bool intersectLocal(const Vec4& origemL, const Vec4& dirL,
                                Vec4& intersectionL, double& tL, Colisao& tipo) const = 0;

    // Intersect no mundo fica centralizado aqui (não reescreve em cada objeto)
    virtual bool intersect(const Vec4& origemW, const Vec4& dirW,
                           Vec4& intersectionW, double& tW, Colisao& tipo) const final
    {
        Vec4 origemL = toLocalPoint(origemW);
        Vec4 dirL = normalize(toLocalVector(dirW));

        Vec4 hitL;
        double tL;
        if (!intersectLocal(origemL, dirL, hitL, tL, tipo)) return false;

        intersectionW = toWorldPoint(hitL);

        // tW consistente no espaço do mundo (assumindo dirW normalizado no render)
        Vec4 dWn = normalize(dirW);
        tW = (intersectionW - origemW).dot(dWn);
        return (tW > 1e-6);
    }

    // Por enquanto você mantém calculaCor em cada classe (sem mexer)
    virtual Color calculaCor(const Vec4& origem, const Vec4& intersection,
                             const Luz& luz, const Luz& luzAmb,
                             const Colisao& tipo, bool isInShadow) const = 0;
};

#endif
