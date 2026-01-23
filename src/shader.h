#pragma once

#include "structs.h"
#include <glad/glad.h>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

class Shader {
public:
	Shader(const std::string& vertexShaderSource, const std::string& fragmentShaderSource);
	~Shader();

	void use() const;
	void setUniform(const std::string& name, int value) const;
	void setUniform(const std::string& name, float value) const;
	void setUniform(const std::string& name, const glm::vec2& value) const;
	void setUniform(const std::string& name, const glm::vec3& value) const;
	void setUniform(const std::string& name, const glm::vec4& value) const;
	void setUniform(const std::string& name, const glm::mat3& value) const;
	void setUniform(const std::string& name, const glm::mat4& value) const;

	void setUniforms(const DirectLight& light) const;
	void setUniforms(const PointLight& light, const int index) const;
	void setUniforms(const SpotLight& light, const int index) const;

	GLuint getProgramID() const {
		return programID;
	};

private:
	GLuint programID;

	GLuint compileShader(const char* shaderSource, GLenum shaderType);
	GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID);
};