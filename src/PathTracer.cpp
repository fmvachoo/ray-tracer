#include "PathTracer.h"
#include <QFile>
#include <QDebug>
#include <algorithm>
#include <numeric>
#include <functional>

// GPU triangle: 3 vertices + 3 normals + material index, padded to std430
struct GPUTriangle {
    float v0[3], pad0;
    float v1[3], pad1;
    float v2[3], pad2;
    float n0[3], pad3;
    float n1[3], pad4;
    float n2[3], pad5;
    int materialIndex;
    int _pad[3];
};

struct GPUMaterial {
    float color[3];
    float roughness;
    float transparency;
    float _pad[3];
};

void PathTracer::init(QOpenGLFunctions_4_3_Core *gl)
{
    m_gl = gl;

    // --- Compute shader ---
    m_computeProgram = new QOpenGLShaderProgram();
    {
        QFile f(":/shaders/pathtracer.comp");
        f.open(QIODevice::ReadOnly);
        QString src = f.readAll();
        if (!m_computeProgram->addShaderFromSourceCode(QOpenGLShader::Compute, src))
            qWarning() << "Compute shader compile error:" << m_computeProgram->log();
        if (!m_computeProgram->link())
            qWarning() << "Compute program link error:" << m_computeProgram->log();
    }

    // --- Tonemap shader ---
    m_tonemapProgram = new QOpenGLShaderProgram();
    {
        QFile vf(":/shaders/tonemap.vert"), ff(":/shaders/tonemap.frag");
        vf.open(QIODevice::ReadOnly);
        ff.open(QIODevice::ReadOnly);
        m_tonemapProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vf.readAll());
        m_tonemapProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, ff.readAll());
        m_tonemapProgram->link();
    }

    // Fullscreen quad
    float quad[] = {
        -1, -1, 0, 0,
         1, -1, 1, 0,
         1,  1, 1, 1,
        -1, -1, 0, 0,
         1,  1, 1, 1,
        -1,  1, 0, 1,
    };
    m_quadVAO.create();
    m_quadVAO.bind();
    m_quadVBO.create();
    m_quadVBO.bind();
    m_quadVBO.allocate(quad, sizeof(quad));
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    m_gl->glEnableVertexAttribArray(1);
    m_gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                reinterpret_cast<void *>(2 * sizeof(float)));
    m_quadVAO.release();

    // SSBOs
    m_gl->glGenBuffers(1, &m_triangleSSBO);
    m_gl->glGenBuffers(1, &m_materialSSBO);
    m_gl->glGenBuffers(1, &m_bvhSSBO);

    // output texture
    m_gl->glGenTextures(1, &m_outputTexture);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_width, m_height, 0,
                       GL_RGBA, GL_FLOAT, nullptr);

    m_initialized = true;
}

void PathTracer::destroy()
{
    if (!m_initialized) return;
    delete m_computeProgram;
    delete m_tonemapProgram;
    m_quadVBO.destroy();
    m_quadVAO.destroy();
    m_gl->glDeleteTextures(1, &m_outputTexture);
    m_gl->glDeleteBuffers(1, &m_triangleSSBO);
    m_gl->glDeleteBuffers(1, &m_materialSSBO);
    m_gl->glDeleteBuffers(1, &m_bvhSSBO);
    m_initialized = false;
}

void PathTracer::resize(int w, int h)
{
    m_width = w;
    m_height = h;
    if (m_initialized) {
        m_gl->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
        m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0,
                           GL_RGBA, GL_FLOAT, nullptr);
    }
}

void PathTracer::buildBVH(const Scene &scene)
{
    // Collect all triangle centroids for a simple SAH-less BVH (midpoint split)
    struct TriRef {
        int globalIdx;
        QVector3D centroid;
    };

    // First build flat triangle list
    QVector<GPUTriangle> allTris;
    int matIdx = 0;
    for (const auto &obj : scene.objects()) {
        const Mesh &m = obj->mesh();
        for (int i = 0; i + 2 < m.indices.size(); i += 3) {
            GPUTriangle t{};
            auto store = [](float dst[3], const QVector3D &v) {
                dst[0] = v.x(); dst[1] = v.y(); dst[2] = v.z();
            };
            store(t.v0, m.vertices[m.indices[i]]);
            store(t.v1, m.vertices[m.indices[i+1]]);
            store(t.v2, m.vertices[m.indices[i+2]]);
            store(t.n0, m.normals[m.indices[i]]);
            store(t.n1, m.normals[m.indices[i+1]]);
            store(t.n2, m.normals[m.indices[i+2]]);
            t.materialIndex = matIdx;
            allTris.append(t);
        }
        matIdx++;
    }

    m_totalTriangles = allTris.size();

    // Build references
    QVector<TriRef> refs(m_totalTriangles);
    for (int i = 0; i < m_totalTriangles; ++i) {
        refs[i].globalIdx = i;
        const auto &t = allTris[i];
        refs[i].centroid = QVector3D(
            (t.v0[0] + t.v1[0] + t.v2[0]) / 3.0f,
            (t.v0[1] + t.v1[1] + t.v2[1]) / 3.0f,
            (t.v0[2] + t.v1[2] + t.v2[2]) / 3.0f);
    }

    m_bvhNodes.clear();
    m_bvhNodes.reserve(2 * m_totalTriangles);

    // Reorder triangles in-place according to BVH build
    QVector<GPUTriangle> orderedTris(m_totalTriangles);
    int orderedCount = 0;

    // Recursive BVH build using lambda
    std::function<int(int, int)> buildNode = [&](int start, int end) -> int {
        int nodeIdx = m_bvhNodes.size();
        m_bvhNodes.append(BVHNode{});

        // compute bounds
        float minX = 1e30f, minY = 1e30f, minZ = 1e30f;
        float maxX = -1e30f, maxY = -1e30f, maxZ = -1e30f;
        for (int i = start; i < end; ++i) {
            const auto &t = allTris[refs[i].globalIdx];
            for (const float *v : {t.v0, t.v1, t.v2}) {
                minX = std::min(minX, v[0]); maxX = std::max(maxX, v[0]);
                minY = std::min(minY, v[1]); maxY = std::max(maxY, v[1]);
                minZ = std::min(minZ, v[2]); maxZ = std::max(maxZ, v[2]);
            }
        }

        m_bvhNodes[nodeIdx].minX = minX;
        m_bvhNodes[nodeIdx].minY = minY;
        m_bvhNodes[nodeIdx].minZ = minZ;
        m_bvhNodes[nodeIdx].maxX = maxX;
        m_bvhNodes[nodeIdx].maxY = maxY;
        m_bvhNodes[nodeIdx].maxZ = maxZ;

        int count = end - start;
        if (count <= 4) {
            // leaf
            int triStart = orderedCount;
            for (int i = start; i < end; ++i) {
                orderedTris[orderedCount++] = allTris[refs[i].globalIdx];
            }
            m_bvhNodes[nodeIdx].leftOrStart = triStart;
            m_bvhNodes[nodeIdx].rightOrCount = count;
            return nodeIdx;
        }

        // split along longest axis
        float extX = maxX - minX, extY = maxY - minY, extZ = maxZ - minZ;
        int axis = 0;
        if (extY > extX && extY > extZ) axis = 1;
        else if (extZ > extX && extZ > extY) axis = 2;

        int mid = (start + end) / 2;
        std::nth_element(refs.begin() + start, refs.begin() + mid, refs.begin() + end,
                         [axis](const TriRef &a, const TriRef &b) {
                             return a.centroid[axis] < b.centroid[axis];
                         });

        m_bvhNodes[nodeIdx].leftOrStart = buildNode(start, mid);
        m_bvhNodes[nodeIdx].rightOrCount = buildNode(mid, end);
        // mark as interior: negative count — we'll use a flag: if rightOrCount >= 0
        // Actually we use a convention: leaf has leftOrStart >= 0 and rightOrCount > 0 but
        // interior also has positive values. We need a separate flag.
        // Convention: if rightOrCount < 0, it's interior with right child = -rightOrCount - 1
        // Wait, let's use a simpler scheme:
        // For leaf: rightOrCount > 0 means leaf, leftOrStart = first tri index
        // For interior: rightOrCount <= 0 means interior node, leftOrStart = left child,
        //   rightOrCount = -(right child + 1)
        // Let's fix:
        int left = m_bvhNodes[nodeIdx].leftOrStart;
        int right = m_bvhNodes[nodeIdx].rightOrCount;
        m_bvhNodes[nodeIdx].leftOrStart = left;
        m_bvhNodes[nodeIdx].rightOrCount = -(right + 1); // negative means interior
        return nodeIdx;
    };

    if (m_totalTriangles > 0)
        buildNode(0, m_totalTriangles);

    // Upload ordered triangles
    m_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_triangleSSBO);
    m_gl->glBufferData(GL_SHADER_STORAGE_BUFFER,
                       orderedCount * sizeof(GPUTriangle),
                       orderedTris.constData(), GL_STATIC_DRAW);
}

void PathTracer::uploadSceneData(const Scene &scene)
{
    buildBVH(scene);

    // Materials
    QVector<GPUMaterial> mats;
    for (const auto &obj : scene.objects()) {
        GPUMaterial m{};
        m.color[0] = obj->material().color.x();
        m.color[1] = obj->material().color.y();
        m.color[2] = obj->material().color.z();
        m.roughness = obj->material().roughness;
        m.transparency = obj->material().transparency;
        mats.append(m);
    }

    m_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_materialSSBO);
    m_gl->glBufferData(GL_SHADER_STORAGE_BUFFER,
                       mats.size() * sizeof(GPUMaterial),
                       mats.constData(), GL_STATIC_DRAW);

    // BVH nodes
    m_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvhSSBO);
    m_gl->glBufferData(GL_SHADER_STORAGE_BUFFER,
                       m_bvhNodes.size() * sizeof(BVHNode),
                       m_bvhNodes.constData(), GL_STATIC_DRAW);
}

void PathTracer::render(const Scene &scene, int samplesPerPixel)
{
    if (!m_initialized) return;

    uploadSceneData(scene);

    m_computeProgram->bind();

    // Bind output image
    m_gl->glBindImageTexture(0, m_outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Bind SSBOs
    m_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_triangleSSBO);
    m_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_materialSSBO);
    m_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_bvhSSBO);

    // Uniforms
    const Camera &cam = scene.camera();
    float aspect = float(m_width) / float(m_height);

    m_computeProgram->setUniformValue("u_resolution", QVector2D(m_width, m_height));
    m_computeProgram->setUniformValue("u_cameraPos", cam.position());
    m_computeProgram->setUniformValue("u_cameraFront", cam.front());
    m_computeProgram->setUniformValue("u_cameraRight", cam.right());
    m_computeProgram->setUniformValue("u_cameraUp", cam.up());
    m_computeProgram->setUniformValue("u_fov", cam.fov());
    m_computeProgram->setUniformValue("u_samples", samplesPerPixel);
    m_computeProgram->setUniformValue("u_numTriangles", m_totalTriangles);
    m_computeProgram->setUniformValue("u_numBVHNodes", (int)m_bvhNodes.size());
    m_computeProgram->setUniformValue("u_seed", (float)(rand() % 10000));

    // Dispatch
    int groupX = (m_width + 15) / 16;
    int groupY = (m_height + 15) / 16;
    m_gl->glDispatchCompute(groupX, groupY, 1);
    m_gl->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    m_computeProgram->release();
}

void PathTracer::displayResult()
{
    if (!m_initialized) return;

    m_gl->glClear(GL_COLOR_BUFFER_BIT);

    m_tonemapProgram->bind();
    m_gl->glActiveTexture(GL_TEXTURE0);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
    m_tonemapProgram->setUniformValue("u_texture", 0);

    m_quadVAO.bind();
    m_gl->glDrawArrays(GL_TRIANGLES, 0, 6);
    m_quadVAO.release();

    m_tonemapProgram->release();
}
