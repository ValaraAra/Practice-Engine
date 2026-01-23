#include "shader.h"
#include "structs.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <format>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
	GLuint vertexShader = compileShader(vertexShaderPath.c_str(), GL_VERTEX_SHADER);
	GLuint fragmentShader = compileShader(fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);

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

void Shader::use() const {
	glUseProgram(programID);
}

// Uniform setting
void Shader::setUniform(const std::string& name, int value) const {
	GLint location = glGetUniformLocation(programID, name.c_str());

	if (location == -1) {
		//std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
		return;
	}

	glUniform1i(location, value);
}

void Shader::setUniform(const std::string& name, float value) const {
	GLint location = glGetUniformLocation(programID, name.c_str());

	if (location == -1) {
		//std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
		return;
	}

	glUniform1f(location, value);
}

void Shader::setUniform(const std::string& name, const glm::vec2& value) const {
	GLint location = glGetUniformLocation(programID, name.c_str());

	if (location == -1) {
		//std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
		return;
	}

	glUniform2fv(location, 1, &value[0]);
}

void Shader::setUniform(const std::string& name, const glm::vec3& value) const {
	GLint location = glGetUniformLocation(programID, name.c_str());

	if (location == -1) {
		//std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
		return;
	}

	glUniform3fv(location, 1, &value[0]);
}

void Shader::setUniform(const std::string& name, const glm::vec4& value) const {
	GLint location = glGetUniformLocation(programID, name.c_str());

	if (location == -1) {
		//std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
		return;
	}

	glUniform4fv(location, 1, &value[0]);
}

void Shader::setUniform(const std::string& name, const glm::mat3& value) const {
	GLint location = glGetUniformLocation(programID, name.c_str());

	if (location == -1) {
		//std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
		return;
	}

	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setUniform(const std::string& name, const glm::mat4& value) const {
	GLint location = glGetUniformLocation(programID, name.c_str());

	if (location == -1) {
		//std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
		return;
	}

	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setUniforms(const DirectLight& light) const {
	setUniform("directLight.direction", light.direction);
	setUniform("directLight.ambient", light.ambient);
	setUniform("directLight.diffuse", light.diffuse);
	setUniform("directLight.specular", light.specular);
}

void Shader::setUniforms(const PointLight& light, const int index) const {
	setUniform(std::format("pointLights[{}].position", index), light.position);
	setUniform(std::format("pointLights[{}].constant", index), light.constant);
	setUniform(std::format("pointLights[{}].linear", index), light.linear);
	setUniform(std::format("pointLights[{}].quadratic", index), light.quadratic);
	setUniform(std::format("pointLights[{}].ambient", index), light.ambient);
	setUniform(std::format("pointLights[{}].diffuse", index), light.diffuse);
	setUniform(std::format("pointLights[{}].specular", index), light.specular);
}

void Shader::setUniforms(const SpotLight& light, const int index) const {
	setUniform(std::format("spotLights[{}].position", index), light.position);
	setUniform(std::format("spotLights[{}].direction", index), light.direction);
	setUniform(std::format("spotLights[{}].cutOff", index), light.cutOff);
	setUniform(std::format("spotLights[{}].outerCutOff", index), light.outerCutOff);
	setUniform(std::format("spotLights[{}].constant", index), light.constant);
	setUniform(std::format("spotLights[{}].linear", index), light.linear);
	setUniform(std::format("spotLights[{}].quadratic", index), light.quadratic);
	setUniform(std::format("spotLights[{}].ambient", index), light.ambient);
	setUniform(std::format("spotLights[{}].diffuse", index), light.diffuse);
	setUniform(std::format("spotLights[{}].specular", index), light.specular);
}

// Complilation and linking
GLuint Shader::compileShader(const char* shaderPath, GLenum shaderType) {
	// Read the Shader code from the file
	std::string ShaderCode;
	std::ifstream ShaderStream(shaderPath, std::ios::in);
	if (ShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << ShaderStream.rdbuf();
		ShaderCode = sstr.str();
		ShaderStream.close();
	}
	else {
		std::cout << "Cannot open shader file: " << shaderPath << std::endl;
		return 0;
	}

	// Compile the Shader
	std::cout << "Compiling shader: " << shaderPath << std::endl;

	GLuint ShaderID = glCreateShader(shaderType);
	char const* SourcePointer = ShaderCode.c_str();
	glShaderSource(ShaderID, 1, &SourcePointer, nullptr);
	glCompileShader(ShaderID);

	// Check the Shader
	GLint Result = GL_FALSE;
	glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Result);

	if (Result == GL_FALSE) {
		int InfoLogLength = 0;
		glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

		if (InfoLogLength > 0) {
			std::vector<char> ShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(ShaderID, InfoLogLength, nullptr, &ShaderErrorMessage[0]);

			std::cout << "Shader compilation error: " << &ShaderErrorMessage[0] << std::endl;
		}

		glDeleteShader(ShaderID);
		return 0;
	}

	std::cout << "Shader compiled successfully: " << shaderPath << std::endl;
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
			glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);

			std::cout << "Shader program linking error: " << &ProgramErrorMessage[0] << std::endl;
		}

		glDeleteProgram(ProgramID);
		return 0;
	}

	// Detach shaders after linking
	glDetachShader(ProgramID, vertexShaderID);
	glDetachShader(ProgramID, fragmentShaderID);

	std::cout << "Shader program linked successfully" << std::endl;
	return ProgramID;
}
