#include "shaderc.hpp"

std::string Shaderc::loadShaderSource(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: " << filepath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Shaderc::loadShader(const char* vertexPath, const char* fragmentPath) {
    std::string vCode = loadShaderSource(vertexPath);
    std::string fCode = loadShaderSource(fragmentPath);

    std::cerr << "[Shaderc] loadShader start: vlen=" << vCode.size() << " flen=" << fCode.size() << std::endl;

    const GLubyte* glver = glGetString(GL_VERSION);
    const GLubyte* glslver = glGetString(GL_SHADING_LANGUAGE_VERSION);
    std::cerr << "GL Version: " << (glver ? (const char*)glver : "(null)")
              << " | GLSL Version: " << (glslver ? (const char*)glslver : "(null)") << std::endl;


    if (vCode.empty()) {
        std::cerr << "ERROR::SHADER::VERTEX::SOURCE_EMPTY: " << vertexPath << std::endl;
        return 0;
    }
    if (fCode.empty()) {
        std::cerr << "ERROR::SHADER::FRAGMENT::SOURCE_EMPTY: " << fragmentPath << std::endl;
        return 0;
    }
    const char* vShaderCode = vCode.c_str();
    const char* fShaderCode = fCode.c_str();

    // Compile vertex shader
    std::cerr << "[Shaderc] compiling vertex shader" << std::endl;
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    if (vertex == 0) { std::cerr << "[Shaderc] glCreateShader returned 0 for vertex\n"; return 0; }
    glShaderSource(vertex, 1, &vShaderCode, 0);
    glCompileShader(vertex);

    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 1024, 0, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        std::cerr << "--- Vertex shader source (" << vertexPath << ") ---\n" << vCode << std::endl;
        GLenum err = glGetError();
        std::cerr << "glGetError after vertex compile: " << err << std::endl;
    }
    else {
        std::cerr << "[Shaderc] vertex compiled successfully (id=" << vertex << ")" << std::endl;
    }

    // Compile fragment shader
    std::cerr << "[Shaderc] compiling fragment shader" << std::endl;
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragment == 0) { std::cerr << "[Shaderc] glCreateShader returned 0 for fragment\n"; return 0; }
    glShaderSource(fragment, 1, &fShaderCode, 0);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 1024, 0, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        std::cerr << "--- Fragment shader source (" << fragmentPath << ") ---\n" << fCode << std::endl;
        GLenum err = glGetError();
        std::cerr << "glGetError after fragment compile: " << err << std::endl;
    }
    else {
        std::cerr << "[Shaderc] fragment compiled successfully (id=" << fragment << ")" << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

	glBindAttribLocation(program, 0, "aPos");
	glBindAttribLocation(program, 1, "aColor");
	glBindAttribLocation(program, 2, "aNormal");
	glBindAttribLocation(program, 3, "aTexCoord");

    std::cerr << "[Shaderc] linking program (id=" << program << ")" << std::endl;
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, 0, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        GLenum err = glGetError();
        std::cerr << "glGetError after program link: " << err << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    std::cerr << "[Shaderc] program linked successfully (id=" << program << ")" << std::endl;

    // Cleanup
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}