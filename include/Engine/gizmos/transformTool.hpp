#ifndef TRANSFORM_TOOL_HPP
#define TRANSFORM_TOOL_HPP

#include <vector>

#include "math/math.hpp"
using namespace NMATH;

#include "Engine/objects/billboard.hpp"

class TransformTool {
public:
	enum Mode {
		MOVE,
		ROTATE,
		SCALE
	};

	TransformTool()
		: mode(MOVE), size(1.0f) {}

	Mode mode;
	float size;

	// Returns true if an axis is grabbed, sets outAxisIndex to 0,1,2 for X,Y,Z
	bool checkAxisGrab(const Vec3d& rayOrigin, const Vec3d& rayDir,
					   const Vec3d& objPosition, const Mat4& view, const Mat4& projection,
					   int viewportX, int viewportY, int viewportW, int viewportH,
					   int& outAxisIndex);

	// Given a ray and an axis index (0=X,1=Y,2=Z), returns the point on the axis closest to the ray
	Vec3d getClosestPointOnAxis(const Vec3d& rayOrigin, const Vec3d& rayDir,
								const Vec3d& objPosition, int axisIndex);

	// Given a ray and a plane normal, returns the intersection point with the plane defined by objPosition and normal
	bool getRayPlaneIntersection(const Vec3d& rayOrigin, const Vec3d& rayDir,
								const Vec3d& planePoint, const Vec3d& planeNormal,
								Vec3d& outIntersection);

	// Draw the gizmo at the given position with the given view/projection matrices
	static void drawGizmo(const Vec3d& objPosition, int grabbedAxisIndex, GLuint shaderProgram, const Mat4& view, const Mat4& projection);
};

#endif