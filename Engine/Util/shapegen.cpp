#include "Util/shapegen.h"

void ShapeGenerator::createCube(float size, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    float h = size / 2.0f;

    glm::vec3 facePositions[6][4] = {
        {{-h,-h,-h},{h,-h,-h},{h,h,-h},{-h,h,-h}}, // back
        {{-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}},     // front
        {{-h,-h,-h},{-h,h,-h},{-h,h,h},{-h,-h,h}}, // left
        {{h,-h,-h},{h,h,-h},{h,h,h},{h,-h,h}},     // right
        {{-h,h,-h},{h,h,-h},{h,h,h},{-h,h,h}},     // top
        {{-h,-h,-h},{h,-h,-h},{h,-h,h},{-h,-h,h}}  // bottom
    };

    glm::vec3 normals[6] = {
        {0,0,-1}, {0,0,1}, {-1,0,0}, {1,0,0}, {0,1,0}, {0,-1,0}
    };

    glm::vec3 color = {1,1,1};

    for(int f=0; f<6; f++){
        unsigned int startIndex = outVertices.size();
        for(int v=0; v<4; v++){
            outVertices.push_back({facePositions[f][v], color, normals[f]});
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

    glm::vec3 positions[4] = {
        {-w, 0, -h}, {w, 0, -h}, {w, 0, h}, {-w, 0, h}
    };

    glm::vec3 normal = {0,1,0};
    glm::vec3 color = {1,1,1};

    unsigned int startIndex = outVertices.size();
    for(int i=0;i<4;i++)
        outVertices.push_back({positions[i], color, normal});

    outIndices.push_back(startIndex);
    outIndices.push_back(startIndex+1);
    outIndices.push_back(startIndex+2);

    outIndices.push_back(startIndex+2);
    outIndices.push_back(startIndex+3);
    outIndices.push_back(startIndex);
}

void ShapeGenerator::createPyramid(float size, float height, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    float h = size / 2.0f;
    glm::vec3 top = {0,height/2.0f,0};

    glm::vec3 base[4] = {
        {-h,-height/2.0f,-h}, {h,-height/2.0f,-h}, {h,-height/2.0f,h}, {-h,-height/2.0f,h}
    };

    glm::vec3 color = {1,1,1};

    // base
    unsigned int startIndex = outVertices.size();
    for(int i=0;i<4;i++)
        outVertices.push_back({base[i], color, {0,-1,0}});

    outIndices.push_back(startIndex);
    outIndices.push_back(startIndex+1);
    outIndices.push_back(startIndex+2);
    outIndices.push_back(startIndex+2);
    outIndices.push_back(startIndex+3);
    outIndices.push_back(startIndex);

    // sides
    for(int i=0;i<4;i++){
        unsigned int idx = outVertices.size();
        glm::vec3 next = base[(i+1)%4];
        // compute normal
        glm::vec3 edge1 = next - base[i];
        glm::vec3 edge2 = top - base[i];
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        outVertices.push_back({base[i], color, normal});
        outVertices.push_back({next, color, normal});
        outVertices.push_back({top, color, normal});

        outIndices.push_back(idx);
        outIndices.push_back(idx+1);
        outIndices.push_back(idx+2);
    }
}

void ShapeGenerator::createSphere(float radius, int segments, int rings, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices) {
    glm::vec3 color = {1,1,1};
    for(int y=0;y<=rings;y++){
        for(int x=0;x<=segments;x++){
            float xSegment = (float)x / segments;
            float ySegment = (float)y / rings;
            float xPos = radius * cos(xSegment * 2.0f * M_PI) * sin(ySegment * M_PI);
            float yPos = radius * cos(ySegment * M_PI);
            float zPos = radius * sin(xSegment * 2.0f * M_PI) * sin(ySegment * M_PI);

            glm::vec3 pos = {xPos,yPos,zPos};
            glm::vec3 normal = glm::normalize(pos);
            outVertices.push_back({pos,color,normal});
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
