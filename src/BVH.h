#pragma once

#include <QVector3D>
#include <QVector>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cfloat>

struct RenderTriangle {
    QVector3D v0, v1, v2;
    QVector3D normal;
    QVector3D color;
    bool emissive = false;
};

struct AABB {
    QVector3D mn{FLT_MAX, FLT_MAX, FLT_MAX};
    QVector3D mx{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    void expand(const QVector3D &p) {
        mn.setX(std::min(mn.x(), p.x()));
        mn.setY(std::min(mn.y(), p.y()));
        mn.setZ(std::min(mn.z(), p.z()));
        mx.setX(std::max(mx.x(), p.x()));
        mx.setY(std::max(mx.y(), p.y()));
        mx.setZ(std::max(mx.z(), p.z()));
    }

    void expand(const AABB &other) {
        expand(other.mn);
        expand(other.mx);
    }

    QVector3D center() const { return (mn + mx) * 0.5f; }

    int longestAxis() const {
        QVector3D d = mx - mn;
        if (d.x() > d.y() && d.x() > d.z()) return 0;
        if (d.y() > d.z()) return 1;
        return 2;
    }

    bool hit(const QVector3D &orig, const QVector3D &invDir, float tMax) const {
        float t1, t2, tmin = 0.0f, tmx = tMax;

        t1 = (mn.x() - orig.x()) * invDir.x();
        t2 = (mx.x() - orig.x()) * invDir.x();
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmx = std::min(tmx, t2);
        if (tmin > tmx) return false;

        t1 = (mn.y() - orig.y()) * invDir.y();
        t2 = (mx.y() - orig.y()) * invDir.y();
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmx = std::min(tmx, t2);
        if (tmin > tmx) return false;

        t1 = (mn.z() - orig.z()) * invDir.z();
        t2 = (mx.z() - orig.z()) * invDir.z();
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmx = std::min(tmx, t2);
        return tmin <= tmx;
    }
};

struct BVHNode {
    AABB box;
    int left = -1;
    int right = -1;
    int triStart = -1;
    int triCount = 0;
    bool isLeaf() const { return triCount > 0; }
};

class BVH {
public:
    void build(QVector<RenderTriangle> &tris);

    // Returns index of hit triangle, -1 if miss
    int intersect(const QVector3D &orig, const QVector3D &dir, float &outT) const;

    const QVector<RenderTriangle> &triangles() const { return m_tris; }

private:
    int buildRecursive(int start, int count);

    static bool triIntersect(const QVector3D &orig, const QVector3D &dir,
                             const RenderTriangle &tri, float &t);

    QVector<RenderTriangle> m_tris;
    std::vector<BVHNode> m_nodes;
};