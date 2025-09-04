#include "Engine/objects/billboard.hpp"

void Billboard::DrawBillboard(const Vec3d& start, const Vec3d& end, float thickness, const Vec3d& color,
    GLuint shader, const Mat4& model, const Mat4& view, const Mat4& projection) {

    Vec3d axis = (end - start).normalized();

    // Pick an up vector that isn’t parallel
    Vec3d up(0, 1, 0);
    if (fabs(axis.dot(up)) > 0.99f) up = Vec3d(1, 0, 0);

    Vec3d side = axis.cross(up).normalized();
    Vec3d offset = side * (thickness * 0.5f);

    // Quad verts
    Vec3d v0 = start - offset;
    Vec3d v1 = start + offset;
    Vec3d v2 = end + offset;
    Vec3d v3 = end - offset;

    float quadVerts[] = {
        (float)v0.x, (float)v0.y, (float)v0.z,
        (float)v1.x, (float)v1.y, (float)v1.z,
        (float)v2.x, (float)v2.y, (float)v2.z,
        (float)v3.x, (float)v3.y, (float)v3.z
    };

    GLuint indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // uniforms
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, model.value_ptr());
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, view.value_ptr());
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, projection.value_ptr());
    glUniform1i(glGetUniformLocation(shader, "useOverrideColor"), 1);
    glUniform3fv(glGetUniformLocation(shader, "overrideColor"), 1, &color[0]);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // cleanup
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}