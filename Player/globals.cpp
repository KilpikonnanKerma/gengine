// Single-definition translation unit for globals shared between editor and player builds.
// Defines symbols that are declared 'extern' in headers (e.g. editor.hpp / sceneManager.cpp).
// Keep this file tiny and safe to include in both targets.
#include <cstdint>

int glShaderType = 0;