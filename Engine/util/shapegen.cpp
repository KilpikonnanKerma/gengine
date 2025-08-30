#include "util/shapegen.hpp"

void ShapeGenerator::createCube(float size, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    float h = size / 2.0f;

    Vec3d facePositions[6][4] = {
        {{-h,-h,-h},{h,-h,-h},{h,h,-h},{-h,h,-h}}, // back
        {{-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}},     // front
        {{-h,-h,-h},{-h,h,-h},{-h,h,h},{-h,-h,h}}, // left
        {{h,-h,-h},{h,h,-h},{h,h,h},{h,-h,h}},     // right
        {{-h,h,-h},{h,h,-h},{h,h,h},{-h,h,h}},     // top
        {{-h,-h,-h},{h,-h,-h},{h,-h,h},{-h,-h,h}}  // bottom
    };

    Vec3d normals[6] = {
        {0,0,-1}, {0,0,1}, {-1,0,0}, {1,0,0}, {0,1,0}, {0,-1,0}
    };

    Vec2d uvs[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    Vec3d color = {1,1,1};

    for(int f=0; f<6; f++){
        unsigned int startIndex = outVertices.size();
        for(int v=0; v<4; v++){
            outVertices.push_back({facePositions[f][v], color, normals[f], uvs[v]});
        }
        // two triangles per face
        outIndices.push_back(startIndex);
        outIndices.push_back(startIndex+1);
        outIndices.push_back(startIndex+2);

        outIndices.push_back(startIndex+2);
        outIndices.push_back(startIndex+3);
        outIndices.push_back(startIndex);
    }
}

void ShapeGenerator::createPlane(float width, float height, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    float w = width / 2.0f;
    float h = height / 2.0f;

    Vec3d positions[4] = {
        {-w, 0, -h}, {w, 0, -h}, {w, 0, h}, {-w, 0, h}
    };

    Vec2d uvs[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    Vec3d normal = {0,1,0};
    Vec3d color = {1,1,1};

    unsigned int startIndex = outVertices.size();
    for(int i=0;i<4;i++)
        outVertices.push_back({positions[i], color, normal, uvs[i]});

    outIndices.push_back(startIndex);
    outIndices.push_back(startIndex+1);
    outIndices.push_back(startIndex+2);

    outIndices.push_back(startIndex+2);
    outIndices.push_back(startIndex+3);
    outIndices.push_back(startIndex);
}

void ShapeGenerator::createPyramid(float size, float height, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    float h = size / 2.0f;
    Vec3d top = {0,height/2.0f,0};

    Vec3d base[4] = {
        {-h,-height/2.0f,-h}, {h,-height/2.0f,-h}, {h,-height/2.0f,h}, {-h,-height/2.0f,h}
    };

    Vec2d baseUVs[4] = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    };

    Vec3d color = {1,1,1};

    // base
    unsigned int startIndex = outVertices.size();
    for(int i=0;i<4;i++)
        outVertices.push_back({base[i], color, {0,-1,0}, baseUVs[i]});

    outIndices.push_back(startIndex);
    outIndices.push_back(startIndex+1);
    outIndices.push_back(startIndex+2);
    outIndices.push_back(startIndex+2);
    outIndices.push_back(startIndex+3);
    outIndices.push_back(startIndex);

    // sides
    for(int i=0;i<4;i++){
        unsigned int idx = outVertices.size();
        Vec3d next = base[(i+1)%4];
        Vec2d uv0 = {0.0f, 0.0f};
        Vec2d uv1 = {1.0f, 0.0f};
        Vec2d uv2 = {0.5f, 1.0f};

        Vec3d edge1 = next - base[i];
        Vec3d edge2 = top - base[i];
        Vec3d normal = edge1.cross(edge2).normalized();

        outVertices.push_back({base[i], color, normal, uv0});
        outVertices.push_back({next,   color, normal, uv1});
        outVertices.push_back({top,    color, normal, uv2});

        outIndices.push_back(idx);
        outIndices.push_back(idx+1);
        outIndices.push_back(idx+2);
    }
}

void ShapeGenerator::createSphere(float radius, int segments, int rings, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    Vec3d color = {1,1,1};
    for(int y=0;y<=rings;y++){
        for(int x=0;x<=segments;x++){
            float xSegment = (float)x / segments;
            float ySegment = (float)y / rings;
            float xPos = radius * NMATH::cos(xSegment * 2.0f * PI) * NMATH::sin(ySegment * PI);
            float yPos = radius * NMATH::cos(ySegment * PI);
            float zPos = radius * NMATH::sin(xSegment * 2.0f * PI) * NMATH::sin(ySegment * PI);

            Vec3d pos = {xPos,yPos,zPos};
            Vec3d normal = pos.normalized();
            Vec2d texCoord = {xSegment, ySegment};

            outVertices.push_back({pos,color,normal,texCoord});
        }
    }

    for(int y=0;y<rings;y++){
        for(int x=0;x<segments;x++){
            int i0 = y * (segments+1) + x;
            int i1 = i0 + segments +1;
            int i2 = i1 +1;
            int i3 = i0 +1;

            outIndices.push_back(i0);
            outIndices.push_back(i1);
            outIndices.push_back(i2);

            outIndices.push_back(i0);
            outIndices.push_back(i2);
            outIndices.push_back(i3);
        }
    }
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);

        // filtering & wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}
