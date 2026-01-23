#pragma once

#include "structs.h"
#include <glad/glad.h>
#include <string>
#include <vector>

class TextureAtlas {
public:
	TextureAtlas(GLsizei width, GLsizei height, GLsizei mipLevels);
	~TextureAtlas();

	void addTexture(std::string name, std::vector<Texel>&& texels);
	void addTexture(std::string name, Texel texel);
	void finish();
	void use();

	GLuint getAtlasID() const {
		return atlasID;
	};

private:
	GLsizei width;
	GLsizei height;
	GLsizei mipLevels;

	GLuint atlasID = 0;
	GLsizei layerCount = 0;
	std::vector<Texture> textures;

	std::vector<GLubyte> getTexelData() const;
};