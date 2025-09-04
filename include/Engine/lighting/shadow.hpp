#ifndef SHADOW_HPP
#define SHADOW_HPP

#include "math/math.hpp"
using namespace NMATH;

#include "glad/glad.h"

class SceneManager;

class Shadow {
public:
	Shadow();
	~Shadow();

	Vec3d lightPos;
	Vec3d lightDir;
	float sz;				// orthographic half-size for directional light
	float nearP;
	float farP;
	int SHADOW_SIZE;

	Mat4 renderDepth(SceneManager* scene, GLuint depthProg);

	GLuint getDepthTexture() const { return depthMapTex; }
	const Mat4& getLightSpaceMatrix() const { return lastLightSpace; }

private:
	GLuint depthMapFBO;
	GLuint depthMapTex;
	Mat4 lastLightSpace;
};

#endif