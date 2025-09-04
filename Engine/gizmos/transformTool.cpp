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
        verts.push_back((float)tip.x); verts.push_back((float)tip.y); verts.push_back((float)tip.z);
        verts.push_back((float)ring[i].x); verts.push_back((float)ring[i].y); verts.push_back((float)ring[i].z);
        verts.push_back((float)ring[ni].x); verts.push_back((float)ring[ni].y); verts.push_back((float)ring[ni].z);
    }

    // Optional base cap (to fill the base): fan triangles baseCenter, ring[ni], ring[i]
    for (int i = 0; i < segments; ++i) {
        int ni = (i + 1) % segments;
        verts.push_back((float)baseCenter.x); verts.push_back((float)baseCenter.y); verts.push_back((float)baseCenter.z);
        verts.push_back((float)ring[ni].x); verts.push_back((float)ring[ni].y); verts.push_back((float)ring[ni].z);
        verts.push_back((float)ring[i].x); verts.push_back((float)ring[i].y); verts.push_back((float)ring[i].z);
    }

    return verts;
}

void TransformTool::drawGizmo(const Vec3d& objPosition, int grabbedAxisIndex, GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
    Vec3d camPos = Vec3d(view.inverse().m[3][0], view.inverse().m[3][1], view.inverse().m[3][2]);
    float dist = (objPosition - camPos).length();

    // Parameters
    float scale = 1.1f;
    float shaftThickness = 0.15f; // diameter; radius = thickness * 0.5
    float arrowSize = 0.225f;
    int segments = 16; // tessellation

    Mat4 model = translate(Mat4(1.0f), objPosition);

    struct Axis { Vec3d dir; Vec3d color; };
    std::vector<Axis> axes;
    axes.push_back(Axis());
    axes.push_back(Axis());
    axes.push_back(Axis());
    axes[0].dir = Vec3d(1,0,0); axes[0].color = Vec3d(1,0,0);
    axes[1].dir = Vec3d(0,1,0); axes[1].color = Vec3d(0,1,0);
    axes[2].dir = Vec3d(0,0,1); axes[2].color = Vec3d(0,0,1);

    for (size_t i = 0; i < axes.size(); i++) {
        Vec3d start(0, 0, 0);
        Vec3d end = axes[i].dir * scale;
        Vec3d color = ((int)i == grabbedAxisIndex) ? Vec3d(1,1,0) : axes[i].color;

        float shaftRadius = shaftThickness * 0.5f;
        Vec3d shaftStart = start;
        Vec3d shaftEnd = end - axes[i].dir.normalized() * (arrowSize * 0.2f);

        std::vector<Vertex> cylVerts;
        std::vector<unsigned int> cylIndices;
        ShapeGenerator::createCylinder(shaftStart, shaftEnd, shaftRadius, segments, cylVerts, cylIndices);

        if (!cylVerts.empty()) {
            // Create and bind objects
            GLuint cylVAO = 0, cylVBO = 0, cylEBO = 0;
            glGenVertexArrays(1, &cylVAO);
            glGenBuffers(1, &cylVBO);
            glGenBuffers(1, &cylEBO);

            glBindVertexArray(cylVAO);

            // Upload vertex buffer (Vertex is a struct with position, color, normal, texCoord)
            glBindBuffer(GL_ARRAY_BUFFER, cylVBO);
            glBufferData(GL_ARRAY_BUFFER, cylVerts.size() * sizeof(Vertex), &cylVerts[0], GL_STATIC_DRAW);

            // Upload index buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylIndices.size() * sizeof(unsigned int), &cylIndices[0], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection.value_ptr());
            glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 1);
            glUniform3fv(glGetUniformLocation(shaderProgram, "overrideColor"), 1, &color[0]);

            glDrawElements(GL_TRIANGLES, (GLsizei)cylIndices.size(), GL_UNSIGNED_INT, (void*)0);

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
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection.value_ptr());
            glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 1);
            glUniform3fv(glGetUniformLocation(shaderProgram, "overrideColor"), 1, &color[0]);

            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(coneVerts.size() / 3));

            glBindVertexArray(0);
            glDeleteBuffers(1, &coneVBO);
            glDeleteVertexArrays(1, &coneVAO);
        }
    }

    glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 0);
    glBindVertexArray(0);
}
