#pragma once

#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include "Scene.h"

class PathTracer {
public:
    PathTracer() = default;

    void init(QOpenGLFunctions_4_3_Core *gl);
    void destroy();

    void resize(int w, int h);
    void render(const Scene &scene, int samplesPerPixel = 64);
    void displayResult();

    bool isReady() const { return m_initialized; }

private:
    void uploadSceneData(const Scene &scene);
    void buildBVH(const Scene &scene);

    QOpenGLFunctions_4_3_Core *m_gl = nullptr;
    bool m_initialized = false;

    // compute shader
    QOpenGLShaderProgram *m_computeProgram = nullptr;

    // tonemap (fullscreen quad)
    QOpenGLShaderProgram *m_tonemapProgram = nullptr;
    QOpenGLVertexArrayObject m_quadVAO;
    QOpenGLBuffer m_quadVBO;

    // textures / buffers
    GLuint m_outputTexture = 0;
    GLuint m_triangleSSBO = 0;
    GLuint m_materialSSBO = 0;
    GLuint m_bvhSSBO = 0;

    int m_width = 800;
    int m_height = 600;

    int m_totalTriangles = 0;

    // BVH node on CPU for upload
    struct BVHNode {
        float minX, minY, minZ;
        int leftOrStart;   // if leaf: start triangle index; else: left child
        float maxX, maxY, maxZ;
        int rightOrCount;  // if leaf: triangle count; else: right child
    };
    QVector<BVHNode> m_bvhNodes;
};