#include "Engine/gizmos/transformTool.hpp"
#include "Engine/objects/shapegen.hpp"
#include <vector>

// Will be moved to shapegen/objects and added as a spawnable object when I'm less lazy
static std::vector<float> generateConeMesh(const Vec3d& baseCenter, const Vec3d& tip, float baseRadius, int segments) {
    std::vector<float> verts;
    if (segments < 3) segments = 3;

    Vec3d dir = (tip - baseCenter).normalized();

    Vec3d up(0, 1, 0);
    if (fabs(dir.dot(up)) > 0.99f) up = Vec3d(1, 0, 0);

    Vec3d side = dir.cross(up).normalized();
    Vec3d up2 = side.cross(dir).normalized();

    std::vector<Vec3d> ring(segments);
    for (int i = 0; i < segments; ++i) {
        float theta = 2.0f * PI * (float)i / (float)segments;
        float c = cosf(theta);
        float s = sinf(theta);
        Vec3d radial = side * c + up2 * s;
        ring[i] = baseCenter + radial * baseRadius;
    }

    // Side triangles: apex, ring[i], ring[i+1]
    for (int i = 0; i < segments; ++i) {
        int ni = (i + 1) % segments;
        verts.push_back((float)tip.x);      verts.push_back((float)tip.y);      verts.push_back((float)tip.z);
        verts.push_back((float)ring[i].x);  verts.push_back((float)ring[i].y);  verts.push_back((float)ring[i].z);
        verts.push_back((float)ring[ni].x); verts.push_back((float)ring[ni].y); verts.push_back((float)ring[ni].z);
    }

    // Optional base cap (to fill the base): fan triangles baseCenter, ring[ni], ring[i]
    for (int i = 0; i < segments; ++i) {
        int ni = (i + 1) % segments;
        verts.push_back((float)baseCenter.x);   verts.push_back((float)baseCenter.y);   verts.push_back((float)baseCenter.z);
        verts.push_back((float)ring[ni].x);     verts.push_back((float)ring[ni].y);     verts.push_back((float)ring[ni].z);
        verts.push_back((float)ring[i].x);      verts.push_back((float)ring[i].y);      verts.push_back((float)ring[i].z);
    }

    return verts;
}

void TransformTool::drawGizmo(const Vec3d& objPosition, int grabbedAxisIndex, GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
    // Query attribute/uniform locations from the supplied program (doesn't require the program to be bound for glGet*).
    GLint locPos = glGetAttribLocation(shaderProgram, "aPos");
    GLint locColor = glGetAttribLocation(shaderProgram, "aColor");
    GLint locNormal = glGetAttribLocation(shaderProgram, "aNormal");
    GLint locModel = glGetUniformLocation(shaderProgram, "model");
    GLint locView = glGetUniformLocation(shaderProgram, "view");
    GLint locProj = glGetUniformLocation(shaderProgram, "projection");
    GLint locUseOverride = glGetUniformLocation(shaderProgram, "useOverrideColor");
    GLint locOverrideColor = glGetUniformLocation(shaderProgram, "overrideColor");

    // It's expected the caller has bound shaderProgram before calling this function.
    // Compute camera pos (for potential scaling) and local transform
    Vec3d camPos = Vec3d(view.inverse().m[3][0], view.inverse().m[3][1], view.inverse().m[3][2]);
    float dist = (objPosition - camPos).length();

    // Parameters
    float scale = 1.1f;
    float shaftThickness = 0.15f; // diameter; radius = thickness * 0.5
    float arrowSize = 0.225f;
    int segments = 16; // tessellation

    Mat4 model = translate(Mat4(1.0f), objPosition);

    struct Axis { Vec3d dir; Vec3d color; };
    std::vector<Axis> axes(3);
    axes[0].dir = Vec3d(1, 0, 0); axes[0].color = Vec3d(1, 0, 0);
    axes[1].dir = Vec3d(0, 1, 0); axes[1].color = Vec3d(0, 1, 0);
    axes[2].dir = Vec3d(0, 0, 1); axes[2].color = Vec3d(0, 0, 1);

    for (size_t i = 0; i < axes.size(); i++) {
        Vec3d start(0, 0, 0);
        Vec3d end = axes[i].dir * scale;
        Vec3d color = ((int)i == grabbedAxisIndex) ? Vec3d(1, 1, 0) : axes[i].color;

        float shaftRadius = shaftThickness * 0.5f;
        Vec3d shaftStart = start;
        Vec3d shaftEnd = end - axes[i].dir.normalized() * (arrowSize * 0.2f);

        std::vector<Vertex> cylVerts;
        std::vector<unsigned int> cylIndices;
        ShapeGenerator::createCylinder(shaftStart, shaftEnd, shaftRadius, segments, cylVerts, cylIndices);

        if (!cylVerts.empty()) {
            GLuint cylVAO = 0, cylVBO = 0, cylEBO = 0;
            glGenVertexArrays(1, &cylVAO);
            glGenBuffers(1, &cylVBO);
            glGenBuffers(1, &cylEBO);

            glBindVertexArray(cylVAO);

            glBindBuffer(GL_ARRAY_BUFFER, cylVBO);
            glBufferData(GL_ARRAY_BUFFER, cylVerts.size() * sizeof(Vertex), &cylVerts[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylIndices.size() * sizeof(unsigned int), &cylIndices[0], GL_STATIC_DRAW);

            // position
            if (locPos >= 0) {
                glEnableVertexAttribArray((GLuint)locPos);
                glVertexAttribPointer((GLuint)locPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
            }
            // color
            if (locColor >= 0) {
                glEnableVertexAttribArray((GLuint)locColor);
                glVertexAttribPointer((GLuint)locColor, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
            }
            // normal
            if (locNormal >= 0) {
                glEnableVertexAttribArray((GLuint)locNormal);
                glVertexAttribPointer((GLuint)locNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            }

            // Set mats & color uniforms (caller must have bound shaderProgram)
            if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, model.value_ptr());
            if (locView >= 0) glUniformMatrix4fv(locView, 1, GL_FALSE, view.value_ptr());
            if (locProj >= 0) glUniformMatrix4fv(locProj, 1, GL_FALSE, projection.value_ptr());
            if (locUseOverride >= 0) glUniform1i(locUseOverride, 1);
            if (locOverrideColor >= 0) glUniform3fv(locOverrideColor, 1, &color[0]);

            glDrawElements(GL_TRIANGLES, (GLsizei)cylIndices.size(), GL_UNSIGNED_INT, (void*)0);

            // cleanup
            if (locPos >= 0) glDisableVertexAttribArray((GLuint)locPos);
            if (locColor >= 0) glDisableVertexAttribArray((GLuint)locColor);
            if (locNormal >= 0) glDisableVertexAttribArray((GLuint)locNormal);
            glBindVertexArray(0);
            glDeleteBuffers(1, &cylVBO);
            glDeleteBuffers(1, &cylEBO);
            glDeleteVertexArrays(1, &cylVAO);
        }

        Vec3d dirNorm = axes[i].dir.normalized();
        Vec3d coneBaseCenter = shaftEnd;
        float coneBaseRadius = arrowSize * 0.5f;
        Vec3d tip = coneBaseCenter + dirNorm * arrowSize;

        std::vector<float> coneVerts = generateConeMesh(coneBaseCenter, tip, coneBaseRadius, segments);

        if (!coneVerts.empty()) {
            GLuint coneVBO = 0, coneVAO = 0;
            glGenVertexArrays(1, &coneVAO);
            glGenBuffers(1, &coneVBO);
            glBindVertexArray(coneVAO);
            glBindBuffer(GL_ARRAY_BUFFER, coneVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * coneVerts.size(), &coneVerts[0], GL_STATIC_DRAW);

            // cone vertex buffer only has positions
            if (locPos >= 0) {
                glEnableVertexAttribArray((GLuint)locPos);
                glVertexAttribPointer((GLuint)locPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            }

            if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, model.value_ptr());
            if (locView >= 0) glUniformMatrix4fv(locView, 1, GL_FALSE, view.value_ptr());
            if (locProj >= 0) glUniformMatrix4fv(locProj, 1, GL_FALSE, projection.value_ptr());
            if (locUseOverride >= 0) glUniform1i(locUseOverride, 1);
            if (locOverrideColor >= 0) glUniform3fv(locOverrideColor, 1, &color[0]);

            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(coneVerts.size() / 3));

            if (locPos >= 0) glDisableVertexAttribArray((GLuint)locPos);
            glBindVertexArray(0);
            glDeleteBuffers(1, &coneVBO);
            glDeleteVertexArrays(1, &coneVAO);
        }
    }

    // restore override uniform off (assumes shader bound by caller)
    if (locUseOverride >= 0) glUniform1i(locUseOverride, 0);
    // done
}