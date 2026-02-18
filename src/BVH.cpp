#include "BVH.h"
#include <QDebug>

void BVH::build(QVector<RenderTriangle> &tris)
{
    m_tris = tris;
    m_nodes.clear();
    m_nodes.reserve(m_tris.size() * 2);

    if (m_tris.isEmpty()) return;

    buildRecursive(0, m_tris.size());
    qDebug() << "BVH built:" << m_nodes.size() << "nodes," << m_tris.size() << "tris";
}

int BVH::buildRecursive(int start, int count)
{
    int nodeIdx = (int)m_nodes.size();
    m_nodes.push_back(BVHNode());

    // Compute bounds
    AABB box;
    for (int i = start; i < start + count; ++i) {
        box.expand(m_tris[i].v0);
        box.expand(m_tris[i].v1);
        box.expand(m_tris[i].v2);
    }
    m_nodes[nodeIdx].box = box;

    if (count <= 4) {
        m_nodes[nodeIdx].triStart = start;
        m_nodes[nodeIdx].triCount = count;
        return nodeIdx;
    }

    int axis = box.longestAxis();

    // Sort triangles by centroid on the longest axis
    auto getAxis = [axis](const QVector3D &v) -> float {
        if (axis == 0) return v.x();
        if (axis == 1) return v.y();
        return v.z();
    };

    std::sort(m_tris.begin() + start, m_tris.begin() + start + count,
              [&](const RenderTriangle &a, const RenderTriangle &b) {
                  QVector3D ca = (a.v0 + a.v1 + a.v2) / 3.0f;
                  QVector3D cb = (b.v0 + b.v1 + b.v2) / 3.0f;
                  return getAxis(ca) < getAxis(cb);
              });

    int half = count / 2;

    int leftIdx = buildRecursive(start, half);
    int rightIdx = buildRecursive(start + half, count - half);

    m_nodes[nodeIdx].left = leftIdx;
    m_nodes[nodeIdx].right = rightIdx;

    return nodeIdx;
}

bool BVH::triIntersect(const QVector3D &orig, const QVector3D &dir,
                        const RenderTriangle &tri, float &t)
{
    const float EPSILON = 1e-6f;
    QVector3D e1 = tri.v1 - tri.v0;
    QVector3D e2 = tri.v2 - tri.v0;
    QVector3D h = QVector3D::crossProduct(dir, e2);
    float a = QVector3D::dotProduct(e1, h);
    if (std::abs(a) < EPSILON) return false;
    float f = 1.0f / a;
    QVector3D s = orig - tri.v0;
    float u = f * QVector3D::dotProduct(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    QVector3D q = QVector3D::crossProduct(s, e1);
    float v = f * QVector3D::dotProduct(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;
    t = f * QVector3D::dotProduct(e2, q);
    return t > EPSILON;
}

int BVH::intersect(const QVector3D &orig, const QVector3D &dir, float &outT) const
{
    if (m_nodes.empty()) return -1;

    QVector3D invDir(1.0f / dir.x(), 1.0f / dir.y(), 1.0f / dir.z());
    outT = FLT_MAX;
    int hitIdx = -1;

    // Stack-based traversal
    int stack[64];
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        int ni = stack[--stackPtr];
        const BVHNode &node = m_nodes[ni];

        if (!node.box.hit(orig, invDir, outT))
            continue;

        if (node.isLeaf()) {
            for (int i = node.triStart; i < node.triStart + node.triCount; ++i) {
                float t;
                if (triIntersect(orig, dir, m_tris[i], t) && t < outT) {
                    outT = t;
                    hitIdx = i;
                }
            }
        } else {
            if (node.left >= 0) stack[stackPtr++] = node.left;
            if (node.right >= 0) stack[stackPtr++] = node.right;
        }
    }

    return hitIdx;
}