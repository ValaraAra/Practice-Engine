#include "shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstdio>
#include <stdexcept>

Shader::Shader(const char* vertexShaderSource, const char* fragmentShaderSource) {
	GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
	GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

	if (vertexShader == 0 || fragmentShader == 0) {
		programID = 0;

		if (vertexShader != 0) {
			glDeleteShader(vertexShader);
		}
		if (fragmentShader != 0) {
			glDeleteShader(fragmentShader);
		}

		throw std::runtime_error("Failed to compile shaders");
	}

	programID = linkProgram(vertexShader, fragmentShader);

	// Delete shaders after linking
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Check if program linking was successful
	if (programID == 0) {
		throw std::runtime_error("Failed to link shader program");
	}
}

Shader::~Shader() {
	glDeleteProgram(programID);
}

void Shader::use() {
	glUseProgram(programID);
}

GLuint Shader::compileShader(const char* shaderSource, GLenum shaderType) {
	// Read the Shader code from the file
	std::string ShaderCode;
	std::ifstream ShaderStream(shaderSource, std::ios::in);
	if (ShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << ShaderStream.rdbuf();
		ShaderCode = sstr.str();
		ShaderStream.close();
	}
	else {
		std::cout << "Cannot open shader file: " << shaderSource << std::endl;
		return 0;
	}

	// Compile the Shader
	std::cout << "Compiling shader: " << shaderSource << std::endl;

	GLuint ShaderID = glCreateShader(shaderType);
	char const* SourcePointer = ShaderCode.c_str();
	glShaderSource(ShaderID, 1, &SourcePointer, NULL);
	glCompileShader(ShaderID);

	// Check the Shader
	GLint Result = GL_FALSE;
	glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Result);

	if (Result == GL_FALSE) {
		int InfoLogLength = 0;
		glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

		if (InfoLogLength > 0) {
			std::vector<char> ShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(ShaderID, InfoLogLength, NULL, &ShaderErrorMessage[0]);

			std::cout << "Shader compilation error: " << &ShaderErrorMessage[0] << std::endl;
		}

		glDeleteShader(ShaderID);
		return 0;
	}

	std::cout << "Shader compiled successfully: " << shaderSource << std::endl << std::endl;
	return ShaderID;
}

GLuint Shader::linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID) {
	// Link the program
	std::cout << "Linking shader program" << std::endl;

	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, vertexShaderID);
	glAttachShader(ProgramID, fragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	GLint Result = GL_FALSE;
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);

	if (Result == GL_FALSE) {
		int InfoLogLength = 0;
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);

		if (InfoLogLength > 0) {
			std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
			glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);

			std::cout << "Shader program linking error: " << &ProgramErrorMessage[0] << std::endl;
		}

		glDeleteProgram(ProgramID);
		return 0;
	}

	// Detach shaders after linking
	glDetachShader(ProgramID, vertexShaderID);
	glDetachShader(ProgramID, fragmentShaderID);

	std::cout << "Shader program linked successfully" << std::endl << std::endl;
	return ProgramID;
}
