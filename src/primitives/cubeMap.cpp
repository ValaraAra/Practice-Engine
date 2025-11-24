#include "cubeMap.h"
#include "structs.h"
#include "shader.h"
#include <iostream>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

CubeMap::CubeMap() {
	setupBuffers();
}

CubeMap::~CubeMap() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

// Maybe add scaling later? (for LODs or something)
void CubeMap::draw(const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	glDepthFunc(GL_LEQUAL);
	glCullFace(GL_FRONT);

	// Set matrix uniforms
	shader.setUniform("view", view);
	shader.setUniform("projection", projection);

	// Draw it
	glBindVertexArray(VAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);
}

void CubeMap::setupBuffers() {
	// Face vertices (right, left, top, bottom, front, back)
	const glm::vec3 faceVertices[6][4] = {
		{{  0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f,  0.5f }, { -0.5f, -0.5f,  0.5f }},
		{{ -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }},
		{{  0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }},
	};

	for (int face = 0; face < 6; face++) {
		unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

		// Add face vertices (4 vertices, quad)
		for (int i = 0; i < 4; i++) {
			vertices.push_back(faceVertices[face][i]);
		}

		// Add face indices (2 triangles, quad)
		indices.push_back(baseIndex + 0);
		indices.push_back(baseIndex + 1);
		indices.push_back(baseIndex + 2);

		indices.push_back(baseIndex + 2);
		indices.push_back(baseIndex + 3);
		indices.push_back(baseIndex + 0);
	}

	std::string facesCubemap[6]{
		"resources/textures/skybox/right.png",
		"resources/textures/skybox/left.png",
		"resources/textures/skybox/top.png",
		"resources/textures/skybox/bottom.png",
		"resources/textures/skybox/front.png",
		"resources/textures/skybox/back.png",
	};

	// Generate and bind buffers and arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind VAO and buffers
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// Fill buffers with data
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind the VAO and buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Cubemap texture setup
	glGenTextures(1, &cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	stbi_set_flip_vertically_on_load(false);
	for (unsigned int i = 0; i < 6; i++) {
		int width, height, nrChannels;
		unsigned char* data = stbi_load(facesCubemap[i].c_str(), &width, &height, &nrChannels, 3);

		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
			std::cout << "Cubemap texture loaded at path: " << facesCubemap[i] << std::endl;
		}
		else {
			std::cout << "Cubemap texture failed to load at path: " << facesCubemap[i] << std::endl;
		}
	}
}
