#include "textureAtlas.h"
#include <iostream>
#include <stdexcept>


TextureAtlas::TextureAtlas(GLsizei width, GLsizei height, GLsizei mipLevels) : width(width), height(height), mipLevels(mipLevels) {
}

TextureAtlas::~TextureAtlas() {
	if (atlasID != 0) {
		glDeleteTextures(1, &atlasID);
	}
}

std::vector<GLubyte> TextureAtlas::getTexelData() const {
	std::vector<GLubyte> data;
	data.reserve(width * height * 4 * textures.size());

	for (const auto& texture : textures) {
		if (texture.texels.size() != static_cast<size_t>(width * height)) {
			throw std::runtime_error("Texture atlas size mismatch");
		}

		for (const auto& texel : texture.texels) {
			data.push_back(texel.r);
			data.push_back(texel.g);
			data.push_back(texel.b);
			data.push_back(texel.a);
		}
	}

	return data;
}

void TextureAtlas::addTexture(std::string name, std::vector<Texel>&& texels) {
	if (texels.size() != static_cast<size_t>(width * height)) {
		throw std::runtime_error("Texture atlas size mismatch");
	}

	Texture texture;
	texture.name = name;
	texture.texels = std::move(texels);

	textures.push_back(std::move(texture));
	layerCount = static_cast<GLsizei>(textures.size());
}

void TextureAtlas::addTexture(std::string name, Texel texel) {
	std::vector<Texel> texels{texel};
	addTexture(name, std::move(texels));
}

void TextureAtlas::finish() {
	if (atlasID != 0) {
		glDeleteTextures(1, &atlasID);
	}

	glGenTextures(1, &atlasID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, atlasID);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGBA8, width, height, layerCount);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, layerCount, GL_RGBA, GL_UNSIGNED_BYTE, getTexelData().data());
	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void TextureAtlas::use() {
	if (atlasID == 0) {
		throw std::runtime_error("Atlas not finished!");
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, atlasID);
}