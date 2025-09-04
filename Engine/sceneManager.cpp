#include "Engine/sceneManager.hpp"
#include <iostream>
#include <sstream>
#include "Engine/util/shaderc.hpp"
#include <sys/stat.h>

extern int glShaderType;
static GLuint s_unlitProgram = 0;
static Shaderc s_shaderCompiler;
static time_t s_unlitVertMtime = 0;
static time_t s_unlitFragMtime = 0;

SceneManager::SceneManager()
		: selectedObject(NULL),
			axisGrabbed(false),
			grabbedAxisIndex(-1),
			objectDrag(false),
			dragPlaneNormal(Vec3d(0.0f)),
			dragInitialPoint(Vec3d(0.0f)),
			dragInitialObjPos(Vec3d(0.0f)),
			axisGrabDistance(0.12f),
			gizmoLineWidth(10.0f),
			axisVAO(0), axisVBO(0),
			lightVAO(0), lightVBO(0),
			selectedLightIndex(-1),
			objCounter(0),
			gridVAO(0), gridVBO(0), gridVertexCount(0)
{

	shadowDepthProgram = s_shaderCompiler.loadShader("shaders/shadows/shadow_depth_vert.glsl", "shaders/shadows/shadow_depth_frag.glsl");
	if (shadowDepthProgram == 0) {
		std::cerr << "[SceneManager] Failed to load shadow depth shader!" << std::endl;
	} else {
		std::cerr << "[SceneManager] shadowDepthProgram id = " << shadowDepthProgram << std::endl;
	}
}

static bool readFileToString(const std::string& path, std::string& out) {
	std::ifstream f(path, std::ios::in | std::ios::binary);
	if (!f.is_open()) return false;
	std::ostringstream ss;
	ss << f.rdbuf();
	out = ss.str();
	return true;
}

void SceneManager::clearScene() {
	for (size_t i = 0; i < objects.size(); i++) {
		delete objects[i];
	}

	for (size_t i = 0; i < lightShadows.size(); i++) {
		delete lightShadows[i];
	}
	
	lightShadows.clear();
	objects.clear();
	selectedObject = nullptr;
	grabbedAxisIndex = -1;
	// Also clear lights when resetting the scene so loadScene replaces them
	lights.clear();
	selectedLightIndex = -1;
}

static void initMeshForType(Object* obj, const std::string& type) {
	if (type == "Cube")         obj->initCube(1.0f);
	else if (type == "Cylinder")obj->initCylinder(0.5f, 2.0f, 16);
	else if (type == "Sphere")  obj->initSphere(0.5f, 16, 16);
	else if (type == "Plane")   obj->initPlane(5.0f, 5.0f);
	else if (type == "Pyramid") obj->initPyramid(1.0f, 1.0f);
	else                        obj->initCube(1.0f);
}

Vec3d closestPointOnLine(const Vec3d& rayOrigin, const Vec3d& rayDir,
							 const Vec3d& linePoint, const Vec3d& lineDir)
{
	// Solve for closest point on 'line' (point=linePoint + lineDir * s) to the ray (origin + rayDir * t)
	Vec3d w0 = rayOrigin - linePoint;
	float a = lineDir.dot(lineDir);           // |u|^2
	float b = lineDir.dot(rayDir);            // u.v
	float c = rayDir.dot(rayDir);             // |v|^2
	float d = lineDir.dot(w0);                // u.(w0)
	float e = rayDir.dot(w0);                 // v.(w0)

	float denom = a * c - b * b;
	const float EPS = 1e-8f;
	float s;
	if (absf(denom) < EPS) {
		// Lines are almost parallel — pick projection of ray origin onto the line
		if (a < EPS) {
			// degenerate lineDir
			return linePoint;
		}
		s = d / a; // projection of w0 onto u
	} else {
		s = (b * e - c * d) / denom;
	}

	return linePoint + lineDir * s;
}

void SceneManager::initLightGizmo() {
	// simple single-point gizmo (we use GL_POINTS) - one vertex at origin, will be translated per-light
	Vec3d v(0.0f,0.0f,0.0f);
	glGenVertexArrays(1, &lightVAO);
	glGenBuffers(1, &lightVBO);

	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3d), &v, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3d),(void*)0);
	glBindVertexArray(0);
}


void SceneManager::drawGizmo(GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
	// save previously bound program
	GLint prevProg = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);

	// bind the gizmo shader/program so TransformTool can set uniforms/attributes correctly
	glUseProgram(shaderProgram);

	// Draw transform gizmo for selected object (TransformTool expects the program to be bound)
	if (selectedObject) {
		TransformTool::drawGizmo(selectedObject->position, grabbedAxisIndex, shaderProgram, view, projection);
	}

	// Draw light gizmos + debug direction ray (unchanged)
	if (lightVAO != 0) {
		glBindVertexArray(lightVAO);
		glPointSize(10.0f * gizmoLineWidth / 2.0f);

		for (size_t i = 0; i < lights.size(); ++i) {
			Mat4 model = translate(Mat4(1.0f), lights[i].position);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model.value_ptr());
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.value_ptr());
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection.value_ptr());

			glUniform3fv(glGetUniformLocation(shaderProgram, "overrideColor"), 1, &lights[i].color[0]);
			glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 1);

			if ((int)i == selectedLightIndex) glPointSize(14.0f * gizmoLineWidth / 2.0f);
			glDrawArrays(GL_POINTS, 0, 1);
			if ((int)i == selectedLightIndex) glPointSize(10.0f * gizmoLineWidth / 2.0f);

			Vec3d p0 = lights[i].position;
			Vec3d dir = lights[i].direction.normalized();
			float rayLen = 0.6f * std::max(1.0f, gizmoLineWidth * 0.1f);
			Vec3d p1 = p0 + dir * rayLen;
			float lineVerts[6] = {
				(float)p0.x, (float)p0.y, (float)p0.z,
				(float)p1.x, (float)p1.y, (float)p1.z
			};

			GLuint lineVAO = 0, lineVBO = 0;
			glGenVertexArrays(1, &lineVAO);
			glGenBuffers(1, &lineVBO);

			glBindVertexArray(lineVAO);
			glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_STATIC_DRAW);

			GLint locPos = glGetAttribLocation(shaderProgram, "aPos");
			if (locPos >= 0) {
				glEnableVertexAttribArray((GLuint)locPos);
				glVertexAttribPointer((GLuint)locPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			}

			glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 1);
			glUniform3fv(glGetUniformLocation(shaderProgram, "overrideColor"), 1, &lights[i].color[0]);

			glDrawArrays(GL_LINES, 0, 2);

			if (locPos >= 0) glDisableVertexAttribArray((GLuint)locPos);
			glBindVertexArray(0);
			glDeleteBuffers(1, &lineVBO);
			glDeleteVertexArrays(1, &lineVAO);
		}

		glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 0);
		glPointSize(1.0f);
		glBindVertexArray(0);
	}

	// restore previously bound program
	glUseProgram((GLuint)prevProg);
}

bool SceneManager::pickLight(const Vec3d& rayOrigin, const Vec3d& rayDir, int& outIndex, float radius) {
	float closestT = FLT_MAX;
	int idx = -1;
	Vec3d dir = rayDir.normalized();
	for (size_t i = 0; i < lights.size(); ++i) {
		const Light& L = lights[i];
		float t = 0.0f;
		if (intersectRaySphere(rayOrigin, dir, L.position, radius * radius, t)) {
			if (t >= 0.0f && t < closestT) {
				closestT = t;
				idx = (int)i;
			}
		}
	}
	if (idx >= 0) {
		outIndex = idx;
		return true;
	}
	return false;
}

bool SceneManager::pickGizmoAxis(const Vec3d& rayOrigin, const Vec3d& rayDir, GizmoAxis& outAxis) {
	if (!selectedObject) return false;
	Vec3d pos = selectedObject->position;

	std::vector<GizmoAxis> axes = {
		{{0,0,0},{1,0,0},{1,0,0},{1,0,0}}, // X
		{{0,0,0},{0,1,0},{0,1,0},{0,1,0}}, // Y
		{{0,0,0},{0,0,1},{0,0,1},{0,0,1}}  // Z
	};

	float bestT = FLT_MAX;
	bool hit = false;
	int hitIndex = -1;

	Vec3d rayDirNorm = rayDir.normalized();

	for (size_t i = 0; i < axes.size(); ++i) {
		GizmoAxis axis = axes[i];

		// world-space endpoints of the axis segment
		Vec3d p1 = pos + axis.start;
		Vec3d p2 = pos + axis.end;

		Vec3d axisDir = p2 - p1;
		float segLen = axisDir.length();
		if (segLen <= 1e-6f) continue;
		Vec3d axisDirNorm = axisDir / segLen;

		// Build a thin quad around the axis that faces the camera (ray)
		// Compute a perpendicular direction to the axis using the ray direction
		Vec3d perp = axisDirNorm.cross(rayDirNorm);
		float perpLen = perp.length();
		if (perpLen < 1e-6f) {
			// ray nearly parallel to axis; pick an arbitrary perpendicular
			perp = axisDirNorm.cross(Vec3d(1.0f, 0.0f, 0.0f));
			perpLen = perp.length();
			if (perpLen < 1e-6f) {
				perp = axisDirNorm.cross(Vec3d(0.0f, 1.0f, 0.0f));
				perpLen = perp.length();
				if (perpLen < 1e-6f) continue;
			}
		}
		Vec3d right = perp / perpLen; // unit perpendicular vector

		// thickness in world space - keep small (axisGrabDistance defined in SceneManager)
		float halfWidth = std::max(0.001f, axisGrabDistance * 0.25f);

		// Build quad vertices (p1->p2 along axis, offset by right)
		Vec3d v0 = p1 + right * halfWidth;
		Vec3d v1 = p2 + right * halfWidth;
		Vec3d v2 = p2 - right * halfWidth;
		Vec3d v3 = p1 - right * halfWidth;

		// Test the ray against the two triangles (v0,v1,v2) and (v0,v2,v3)
		float tHit = 0.0f;
		bool triHit = false;
		float tTemp = 0.0f;

		if (IntersectRayTriangle(rayOrigin, rayDirNorm, v0, v1, v2, tTemp)) {
			triHit = true;
			tHit = tTemp;
		}
		if (IntersectRayTriangle(rayOrigin, rayDirNorm, v0, v2, v3, tTemp)) {
			if (!triHit || tTemp < tHit) {
				triHit = true;
				tHit = tTemp;
			}
		}

		if (triHit) {
			Vec3d closest = closestPointOnLine(rayOrigin, rayDir, p1, axisDir);
			float proj = (closest - p1).dot(axisDirNorm);

			// choose the closest hit in world space
			if (tHit >= 0.0f && tHit < bestT) {
				bestT = tHit;
				hit = true;
				hitIndex = (int)i;

				GizmoAxis worldAxis = axis;
				worldAxis.start = p1;
				worldAxis.end = p2;
				worldAxis.dir = axisDir;
				worldAxis.initialProj = proj;
				worldAxis.initialObjPos = selectedObject ? selectedObject->position : Vec3d(0.0f);
				outAxis = worldAxis;
			}
		}
	}

	if (hit) grabbedAxisIndex = hitIndex;
	else grabbedAxisIndex = -1;

	return hit;
}

void SceneManager::dragSelectedObject(const Vec3d& rayOrigin, const Vec3d& rayDir){
	if(!axisGrabbed || !selectedObject || grabbedAxisIndex < 0) return;

	// world-space axis start and direction captured at pick time
	Vec3d start = grabbedAxis.start;
	Vec3d axisDir = grabbedAxis.dir;
	float len = axisDir.length();
	if (len < 1e-6f) return;
	Vec3d axisDirNorm = axisDir / len;

	// current closest point on the infinite axis line to the mouse ray
	Vec3d currentClosest = closestPointOnLine(rayOrigin, rayDir, start, axisDir);

	// reconstruct the original picked point on the axis (where user clicked)
	Vec3d pickedWorld = start + axisDirNorm * grabbedAxis.initialProj;

	// Signed distance along the axis from the original picked point to the current closest point.
	// Using the scalar projection avoids sign/plane problems and keeps motion constrained to the axis.
	float signedDelta = (pickedWorld - currentClosest).dot(axisDirNorm);

	// small deadzone to avoid jitter
	const float DEADZONE = 1e-4f;
	if (absf(signedDelta) < DEADZONE) signedDelta = 0.0f;

	// Apply movement only along the axis, preserving object's original position at pick time
	Vec3d newPos = grabbedAxis.initialObjPos + axisDirNorm * signedDelta;
	selectedObject->position = newPos;
}

Object* SceneManager::addObject(const std::string& type, const std::string& name) {
	Object* obj = new Object();
	initMeshForType(obj, type);
	obj->name = name;
	objects.push_back(obj);
	return obj;
}

void SceneManager::removeObject(Object* objPtr) {
	for (std::vector<Object*>::iterator it = objects.begin(); it != objects.end(); ) {
		if ((*it) == objPtr) {
			it = objects.erase(it);
		} else {
			++it;
		}
	}

	delete objPtr;
}

void SceneManager::removeLight(int index) {
	if (index >= 0 && index < (int)lights.size()) {
		lights.erase(lights.begin() + index);

		if (index >= 0 && index < (int)lightShadows.size()) {
			delete lightShadows[index];
			lightShadows.erase(lightShadows.begin() + index);
		}
	}
}

void SceneManager::addLight(const Light& light){
	lights.push_back(light);
	// ensure gizmo resources exist when lights are present
	if (lightVAO == 0) initLightGizmo();

	Shadow* sh = new Shadow();
	sh->lightPos = light.position;
	sh->lightDir = light.direction;

	sh->sz = 5.0f;
	sh->nearP = 0.1f;
	sh->farP = 100.0f;
	lightShadows.push_back(sh);
}

void SceneManager::update(float deltaTime) {}

void SceneManager::render(GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
	const char* unlitVertPath = "shaders/unlit/vertex.glsl";
	const char* unlitFragPath = "shaders/unlit/fragment.glsl";

	auto getMTime = [](const char* path)->time_t {
		struct stat st;
		if (stat(path, &st) == 0) return st.st_mtime;
		return 0;
	};

	if (glShaderType == 1) {
		time_t vm = getMTime(unlitVertPath);
		time_t fm = getMTime(unlitFragPath);
		if (s_unlitProgram == 0 || vm != s_unlitVertMtime || fm != s_unlitFragMtime) {
			if (s_unlitProgram != 0) {
				glDeleteProgram(s_unlitProgram);
				s_unlitProgram = 0;
			}
			GLuint prog = s_shaderCompiler.loadShader(unlitVertPath, unlitFragPath);
			if (prog != 0) {
				s_unlitProgram = prog;
				s_unlitVertMtime = vm;
				s_unlitFragMtime = fm;
				std::cerr << "[SceneManager] Loaded unlit shader id=" << prog << std::endl;
			}
			else {
				std::cerr << "[SceneManager] Failed to load unlit shader, falling back to lit." << std::endl;
			}
		}
	}

	// Detect depth-only pass (renderDepth calls scene->render with depthProgram)
	bool depthPass = (shaderProgram == shadowDepthProgram);

	GLuint activeProgram = (glShaderType == 1 && s_unlitProgram != 0 && !depthPass) ? s_unlitProgram : shaderProgram;
	glUseProgram(activeProgram);

	// store for external users (grid/gizmo drawing callers)
	this->lastActiveProgram = activeProgram;

	// --- Ensure axis VAO/VBO exist (used for transform gizmo helper drawing in render)
	if (axisVAO == 0) {
		// create a simple three-axis line buffer (0->X, 0->Y, 0->Z)
		Vec3d axisVerts[6] = {
			Vec3d(0.0f,0.0f,0.0f), Vec3d(1.0f,0.0f,0.0f), // X
			Vec3d(0.0f,0.0f,0.0f), Vec3d(0.0f,1.0f,0.0f), // Y
			// will draw Z as two verts in second draw call (we store both pairs)
		};
		// expand to 6 verts (X,Y,Z)
		Vec3d allVerts[6] = {
			Vec3d(0.0f,0.0f,0.0f), Vec3d(1.0f,0.0f,0.0f),
			Vec3d(0.0f,0.0f,0.0f), Vec3d(0.0f,1.0f,0.0f),
			Vec3d(0.0f,0.0f,0.0f), Vec3d(0.0f,0.0f,1.0f)
		};
		glGenVertexArrays(1, &axisVAO);
		glGenBuffers(1, &axisVBO);
		glBindVertexArray(axisVAO);
		glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(allVerts), allVerts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3d), (void*)0);
		glBindVertexArray(0);
	}

	float sceneMaxY = -1e9f;
	bool hasSceneObjects = false;
	for (size_t oi = 0; oi < objects.size(); ++oi) {
		Object* o = objects[oi];
		if (!o) continue;
		hasSceneObjects = true;
		sceneMaxY = std::max(sceneMaxY, o->position.y);
	}
	if (!hasSceneObjects) sceneMaxY = 0.0f; // fallback

	// Only generate shadow maps during the regular (non-depth) render
	if (!depthPass) {
		int texBase = 4; // choose starting texture unit (0 = diffuse texture)
		int dirShadowCount = 0;
		for (size_t li = 0; li < lights.size() && dirShadowCount < MAX_DIR_SHADOWS; ++li) {
			Light& L = lights[li];
			if (L.type != LightType::Directional) continue;
			// update shadow params
			if (li < lightShadows.size() && lightShadows[li]) {
				Shadow* sh = lightShadows[li];
				sh->lightPos = L.position;
				sh->lightDir = L.direction;
				// render depth (this will bind the depth program + FBO)
				Mat4 ls = sh->renderDepth(this, shadowDepthProgram);

				// renderDepth may have changed the bound program/framebuffer; re-bind our active program
				glUseProgram(activeProgram);

				// bind depth texture to a texture unit
				int texUnit = texBase + dirShadowCount;
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_2D, sh->getDepthTexture());
				// set sampler and light-space uniform arrays in the main shader (activeProgram)
				char buf[64];
				snprintf(buf, sizeof(buf), "shadowMap[%d]", dirShadowCount);
				GLint locS = glGetUniformLocation(activeProgram, buf);
				if (locS >= 0) glUniform1i(locS, texUnit);

				snprintf(buf, sizeof(buf), "lightSpaceMatrix[%d]", dirShadowCount);
				GLint locM = glGetUniformLocation(activeProgram, buf);
				if (locM >= 0) glUniformMatrix4fv(locM, 1, GL_FALSE, ls.value_ptr());

				// set per-shadow cull height so fragments above that height don't sample shadows
				snprintf(buf, sizeof(buf), "uShadowCullHeight[%d]", dirShadowCount);
				GLint locCull = glGetUniformLocation(activeProgram, buf);
				if (locCull >= 0) {
					// margin above highest object where shadows are considered meaningful
					float margin = 5.0f;
					glUniform1f(locCull, sceneMaxY + margin);
				}

				// Inform shader of shadow map resolution (used for PCF)
				GLint locSz = glGetUniformLocation(activeProgram, "uShadowMapSize");
				if (locSz >= 0) glUniform1f(locSz, (float)sh->SHADOW_SIZE);

				++dirShadowCount;
			}
		}
	}

	// Set per-light uniforms (direction/color/intensity) on the active program
	int dirCount = 0, pointCount = 0;
	for (size_t i = 0; i < lights.size(); i++) {
		Light& l = lights[i];
		if (l.type == LightType::Directional) {
			std::ostringstream ss; ss << dirCount; std::string idx = ss.str();
			std::string locDirs = std::string("dirLightDirs[") + idx + "]";
			std::string locColors = std::string("dirLightColors[") + idx + "]";
			std::string locInts = std::string("dirLightIntensities[") + idx + "]";
			glUniform3fv(glGetUniformLocation(activeProgram, locDirs.c_str()), 1, &l.direction[0]);
			glUniform3fv(glGetUniformLocation(activeProgram, locColors.c_str()), 1, &l.color[0]);
			glUniform1f(glGetUniformLocation(activeProgram, locInts.c_str()), l.intensity);
			dirCount++;
		} else if (l.type == LightType::Point) {
			std::ostringstream ss; ss << pointCount; std::string idx = ss.str();
			std::string locPos = std::string("pointLightPositions[") + idx + "]";
			std::string locCol = std::string("pointLightColors[") + idx + "]";
			std::string locInt = std::string("pointLightIntensities[") + idx + "]";
			glUniform3fv(glGetUniformLocation(activeProgram, locPos.c_str()), 1, &l.position[0]);
			glUniform3fv(glGetUniformLocation(activeProgram, locCol.c_str()), 1, &l.color[0]);
			glUniform1f(glGetUniformLocation(activeProgram, locInt.c_str()), l.intensity);
			pointCount++;
		}
	}
	// set counts on the active program
	glUniform1i(glGetUniformLocation(activeProgram, "uNumDirLights"), dirCount);
	glUniform1i(glGetUniformLocation(activeProgram, "uNumPointLights"), pointCount);

	unsigned int modelLoc = glGetUniformLocation(activeProgram, "model");
	unsigned int viewLoc  = glGetUniformLocation(activeProgram, "view");
	unsigned int projLoc  = glGetUniformLocation(activeProgram, "projection");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.value_ptr());
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection.value_ptr());

	// Draw scene objects (both regular and depth passes)
	for (size_t oi = 0; oi < objects.size(); ++oi) {
		Object* obj = objects[oi];
		Mat4 model = obj->getModelMatrix();
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.value_ptr());

		if (!depthPass) {
			if (obj->textureID != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, obj->textureID);
				glUniform1i(glGetUniformLocation(activeProgram, "uTexture"), 0);
				glUniform1i(glGetUniformLocation(activeProgram, "useTexture"), 1);
			} else {
				glUniform1i(glGetUniformLocation(activeProgram, "useTexture"), 0);
			}

			// ensure override is off for normal object draw
			glUniform1i(glGetUniformLocation(activeProgram, "useOverrideColor"), 0);
		}

		glBindVertexArray(obj->VAO);
		glDrawElements(GL_TRIANGLES, (GLsizei)obj->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	// If we were called as a depth pass, return now — do not draw gizmos, grid, or light gizmos into shadow maps.

	if (depthPass) return;

	if (selectedObject) {
		glBindVertexArray(axisVAO);

		GLboolean prevDepthMask = GL_TRUE;
		glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
		GLboolean wasDepthEnabled = glIsEnabled(GL_DEPTH_TEST);

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		TransformTool::drawGizmo(selectedObject->position, grabbedAxisIndex, shaderProgram, view, projection);

		glDepthMask(prevDepthMask);
		if (wasDepthEnabled) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);

		glBindVertexArray(0);
	}

	// Draw light gizmos (always draw, even if no object is selected)
	if (lightVAO != 0) {
		glBindVertexArray(lightVAO);
		glPointSize(10.0f);
		for (size_t i = 0; i < lights.size(); ++i) {
			// draw point at light position
			Mat4 model = translate(Mat4(1.0f), lights[i].position);
			glUniformMatrix4fv(glGetUniformLocation(activeProgram,"model"),1,GL_FALSE, model.value_ptr());
			glUniform3fv(glGetUniformLocation(activeProgram,"overrideColor"),1,&lights[i].color[0]);
			glUniform1i(glGetUniformLocation(activeProgram,"useOverrideColor"), 1);

			// ensure the lightVBO contains a single point when drawing point
			Vec3d pt = lights[i].position;
			glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3d), &Vec3d(0.0f,0.0f,0.0f), GL_DYNAMIC_DRAW);
			// draw point (will be transformed by model uniform)
			if ((int)i == selectedLightIndex) glPointSize(14.0f);
			glDrawArrays(GL_POINTS, 0, 1);
			if ((int)i == selectedLightIndex) glPointSize(10.0f);

			// --- Debug direction ray: update same VBO with two positions in world-space and draw as lines ---
			Vec3d p0 = lights[i].position;
			Vec3d dir = lights[i].direction.normalized();
			float rayLen = 0.6f * std::max(1.0f, gizmoLineWidth * 0.1f);
			Vec3d p1 = p0 + dir * rayLen;
			Vec3d linePts[2] = { p0, p1 };

			// Because the shader multiplies by model, we set model to identity for world-space line verts
			Mat4 identity = Mat4(1.0f);
			glUniformMatrix4fv(glGetUniformLocation(activeProgram,"model"),1,GL_FALSE, identity.value_ptr());

			glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(linePts), linePts, GL_DYNAMIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3d),(void*)0);

			glUniform1i(glGetUniformLocation(activeProgram, "useOverrideColor"), 1);
			glUniform3fv(glGetUniformLocation(activeProgram, "overrideColor"), 1, &lights[i].color[0]);

			glDrawArrays(GL_LINES, 0, 2);

			// restore VBO to single point  (so next point draw works)
			Vec3d origin(0.0f,0.0f,0.0f);
			glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3d), &origin, GL_DYNAMIC_DRAW);
		}
		glUniform1i(glGetUniformLocation(activeProgram,"useOverrideColor"), 0);
		glPointSize(1.0f);
		glBindVertexArray(0);
	}
}
Object* SceneManager::pickObject(const Vec3d& rayOrigin, const Vec3d& rayDir) {
	float bestDist = FLT_MAX;
	Object* picked = nullptr;

	for (size_t oi = 0; oi < objects.size(); ++oi) {
		Object* obj = objects[oi];
		if (!obj) continue;
		if (obj->indices.size() < 3 || obj->vertices.size() == 0) continue;

		// Transform ray into object local space
		Mat4 model = obj->getModelMatrix();
		Mat4 invModel = model.inverse();
		Vec3d localOrig = invModel.transformPoint(rayOrigin);
		Vec3d localDir = invModel.transformDir(rayDir).normalized();

		for (size_t idx = 0; idx + 2 < obj->indices.size(); idx += 3) {
			unsigned int i0 = obj->indices[idx + 0];
			unsigned int i1 = obj->indices[idx + 1];
			unsigned int i2 = obj->indices[idx + 2];
			if (i0 >= obj->vertices.size() || i1 >= obj->vertices.size() || i2 >= obj->vertices.size())
				continue;
			const Vec3d& v0 = obj->vertices[i0].pos;
			const Vec3d& v1 = obj->vertices[i1].pos;
			const Vec3d& v2 = obj->vertices[i2].pos;

			float tLocal = 0.0f;
			if (IntersectRayTriangle(localOrig, localDir, v0, v1, v2, tLocal)) {
				Vec3d localHit = localOrig + localDir * tLocal;
				Vec3d worldHit = model.transformPoint(localHit);
				float distWorld = (worldHit - rayOrigin).length();
				if (distWorld < bestDist) {
					bestDist = distWorld;
					picked = obj;
				}
			}
		}
	}

	return picked;
}


void SceneManager::initGrid(int gridSize, float spacing) {
	std::vector<Vec3d> gridVertices;

	for (int i = -gridSize; i <= gridSize; i++) {
		gridVertices.push_back(Vec3d(i * spacing, 0.0f, -gridSize * spacing));
		gridVertices.push_back(Vec3d(i * spacing, 0.0f,  gridSize * spacing));

		gridVertices.push_back(Vec3d(-gridSize * spacing, 0.0f, i * spacing));
		gridVertices.push_back(Vec3d( gridSize * spacing, 0.0f, i * spacing));
	}

	gridVertexCount = gridVertices.size();

	glGenVertexArrays(1, &gridVAO);
	glGenBuffers(1, &gridVBO);

	glBindVertexArray(gridVAO);
	glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
	glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(Vec3d),
	gridVertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3d), (void*)0);

	glBindVertexArray(0);
}

void SceneManager::drawGrid(GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
	if (gridVAO == 0) return;

	Mat4 model = Mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model.value_ptr());
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.value_ptr());
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection.value_ptr());

	glBindVertexArray(gridVAO);
	glDrawArrays(GL_LINES, 0, gridVertexCount);
	glBindVertexArray(0);
}

void SceneManager::saveScene(const std::string& path) {
	json j;
	j["objects"] = json::array();

	for (size_t oi = 0; oi < objects.size(); ++oi) {
		Object* obj = objects[oi];
		json o;
		o["name"] = obj->name;
		o["type"] = obj->type.empty() ? "Cube" : obj->type;
		json posArr = json::array(); posArr.push_back(obj->position.x); posArr.push_back(obj->position.y); posArr.push_back(obj->position.z);
		json rotArr = json::array(); rotArr.push_back(obj->rotation.x); rotArr.push_back(obj->rotation.y); rotArr.push_back(obj->rotation.z);
		json sclArr = json::array(); sclArr.push_back(obj->scale.x); sclArr.push_back(obj->scale.y); sclArr.push_back(obj->scale.z);
		o["position"] = posArr;
		o["rotation"] = rotArr;
		o["scale"] = sclArr;
		o["texturePath"] = obj->texturePath;
		j["objects"].push_back(o);
	}


	j["lights"] = json::array();

	for (size_t li = 0; li < lights.size(); ++li) {
		Light light = lights[li];
		json l;
		l["type"] = (light.type == LightType::Directional) ? "Directional" : "Point";
		l["position"] = { light.position.x, light.position.y, light.position.z };
		l["color"] = { light.color.x, light.color.y, light.color.z };
		l["intensity"] = light.intensity;
		j["lights"].push_back(l);
	}

	std::ofstream file(path);
	if (file.is_open()) {
		file << j.dump(4);
	} else {
		std::cerr << "[saveScene] Failed to open " << path << " for writing\n";
	}
}

void SceneManager::loadScene(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "[loadScene] Failed to open " << path << "\n";
		return;
	}

	json j;
	try {
		file >> j;
	} catch (const std::exception& e) {
		std::cerr << "[loadScene] JSON parse error: " << e.what() << "\n";
		return;
	}

	clearScene();

	if (j.find("objects") == j.end() || !j["objects"].is_array()) {
		std::cerr << "[loadScene] No 'objects' array in scene\n";
		return;
	}

	for (size_t ji = 0; ji < j["objects"].size(); ++ji) {
		const json& objJson = j["objects"][ji];
		std::string type = objJson.value("type", "Cube");
		std::string name = objJson.value("name", type);

		Vec3d pos(0.0f), rot(0.0f), scl(1.0f);
		if (objJson.find("position") != objJson.end() && objJson["position"].size() == 3) {
			pos = Vec3d(objJson["position"][0], objJson["position"][1], objJson["position"][2]);
		}
		if (objJson.find("rotation") != objJson.end() && objJson["rotation"].size() == 3) {
			rot = Vec3d(objJson["rotation"][0], objJson["rotation"][1], objJson["rotation"][2]);
		}
		if (objJson.find("scale") != objJson.end() && objJson["scale"].size() == 3) {
			scl = Vec3d(objJson["scale"][0], objJson["scale"][1], objJson["scale"][2]);
		}
		std::string texPath = objJson.value("texturePath", "");

		Object* o = new Object();
		o->name = name;
		o->type = type;

		initMeshForType(o, type);

		o->position = pos;
		o->rotation = rot;
		o->scale    = scl;

		if (!texPath.empty()) {
			o->texture(texPath);
		}

		objects.push_back(o);
	}

	if (j.find("lights") == j.end() || !j["lights"].is_array()) {
		std::cerr << "[loadScene] No 'lights' array in scene\n";
	}

	for (size_t li = 0; li < j["lights"].size(); ++li) {
		Vec3d color = {1,1,1};
		Vec3d pos = {0,0,0};
		LightType type = LightType::Directional;

		const json& lightJson = j["lights"][li];
		if (lightJson.find("type") != lightJson.end()) {
			std::string typeStr = lightJson["type"];
			if (typeStr == "Directional") type = LightType::Directional;
			else type = LightType::Point;
		}
		if (lightJson.find("position") != lightJson.end() && lightJson["position"].size() == 3) {
			pos = Vec3d(lightJson["position"][0], lightJson["position"][1], lightJson["position"][2]);
		}
		if (lightJson.find("color") != lightJson.end() && lightJson["color"].size() == 3) {
			color = Vec3d(
				static_cast<float>(lightJson["color"][0]),
				static_cast<float>(lightJson["color"][1]),
				static_cast<float>(lightJson["color"][2])
			);
		}
		float intensity = lightJson.value("intensity", 1.0f);
		Light light(type, color, intensity);
		light.position = pos;
		lights.push_back(light);
		std::string tname = (type == LightType::Directional) ? "Directional" : "Point";
		std::cerr << "[loadScene] loaded light[" << li << "] type=" << tname
				  << " pos=(" << pos.x << "," << pos.y << "," << pos.z << ")"
				  << " color=(" << color.x << "," << color.y << "," << color.z << ")"
				  << " intensity=" << intensity << std::endl;

		Shadow* sh = new Shadow();
		sh->lightPos = light.position;
		sh->lightDir = light.direction;
		sh->sz = 20.0f;
		sh->nearP = 1.0f;
		sh->farP = 60.0f;
		lightShadows.push_back(sh);
	}
}