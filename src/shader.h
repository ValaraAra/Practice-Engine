#pragma once

#include <glad/glad.h>

class Shader {
public:
	Shader(const char* vertexShaderSource, const char* fragmentShaderSource);
	~Shader();

	void use();
	GLuint getProgramID() const {
		return programID;
	};

private:
	GLuint programID;

	GLuint compileShader(const char* shaderSource, GLenum shaderType);
	GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID);
};