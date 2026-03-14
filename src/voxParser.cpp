#include "voxParser.h"
#include "structs.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace {
	static Voxel matchVoxelType(const uint8_t colorIndex) {
		switch (colorIndex) {
		case 0: return Voxel{ VoxelType::EMPTY };
		case 1: return Voxel{ VoxelType::ERROR };
		case 2: return Voxel{ VoxelType::STONE };
		case 3: return Voxel{ VoxelType::DIRT };
		case 4: return Voxel{ VoxelType::GRASS };
		case 5: return Voxel{ VoxelType::WATER };
		case 6: return Voxel{ VoxelType::SAND };
		case 7: return Voxel{ VoxelType::WOOD };
		case 8: return Voxel{ VoxelType::LEAVES };
		default: return Voxel{ VoxelType::ERROR };
		}
	}

	static VoxParser::VoxVoxels parseChunkVoxels(const VoxParser::VoxChunk& chunk) {
		VoxParser::VoxVoxels result;

		// Num voxels
		if (chunk.data.size() < 4) {
			throw std::runtime_error("Invalid XYZI chunk data size!");
		}

		std::memcpy(&result.numVoxels, chunk.data.data(), 4);

		// Voxels
		if (chunk.data.size() != (4 + result.numVoxels * 4)) {
			throw std::runtime_error("Invalid XYZI chunk data size for voxels!");
		}

		if (result.numVoxels > 0) {
			result.voxels.resize(result.numVoxels);
			std::memcpy(result.voxels.data(), chunk.data.data() + 4, result.numVoxels * 4);
		}

		return result;
	}

	static VoxParser::VoxSize parseChunkSize(const VoxParser::VoxChunk& chunk) {
		VoxParser::VoxSize result;

		if (chunk.data.size() != 12) {
			throw std::runtime_error("Invalid SIZE chunk data size!");
		}

		std::memcpy(&result.x, chunk.data.data(), 4);
		std::memcpy(&result.y, chunk.data.data() + 4, 4);
		std::memcpy(&result.z, chunk.data.data() + 8, 4);

		return result;
	}

	static VoxParser::VoxChunk readChunk(std::ifstream& fileStream) {
		VoxParser::VoxChunk chunk;

		// Read chunk info (ID, size, child size)
		if (!fileStream.read(chunk.id, 4)) {
			throw std::runtime_error("Failed to read chunk ID!");
		}
		if (!fileStream.read(reinterpret_cast<char*>(&chunk.size), 4)) {
			throw std::runtime_error("Failed to read chunk size!");
		}
		if (!fileStream.read(reinterpret_cast<char*>(&chunk.childSize), 4)) {
			throw std::runtime_error("Failed to read chunk chilren size!");
		}

		// Check for invalid size
		if (chunk.size < 0) {
			throw std::runtime_error("Invalid chunk size!");
		}
		if (chunk.childSize < 0) {
			throw std::runtime_error("Invalid chunk child size!");
		}

		// Read chunk data
		if (chunk.size > 0) {
			chunk.data.resize(static_cast<size_t>(chunk.size));
			if (!fileStream.read(reinterpret_cast<char*>(chunk.data.data()), chunk.size)) {
				throw std::runtime_error("Failed to read chunk data!");
			}
		}

		return chunk;
	}

	static std::vector<VoxParser::VoxChunk> readChildren(std::ifstream& fileStream, std::streamoff bytes) {
		std::vector<VoxParser::VoxChunk> children;

		const std::streampos start = fileStream.tellg();

		// Check for invalid size
		if (start == std::streampos(-1)) {
			throw std::runtime_error("Invalid stream position when reading children!");
		}

		const std::streampos end = start + bytes;

		// Read child chunks
		while (true) {
			const std::streampos current = fileStream.tellg();
			if (current == std::streampos(-1) || current >= end) {
				break;
			}

			VoxParser::VoxChunk chunk = readChunk(fileStream);
			if (chunk.childSize > 0) {
				chunk.children = readChildren(fileStream, static_cast<std::streamoff>(chunk.childSize));
			}
			children.push_back(std::move(chunk));
		}

		// Move to end just in case
		fileStream.seekg(end);
		return children;
	}
}

namespace VoxParser {
	bool loadVoxModel(const std::string& filename, VoxelModel& model) {
		try {
			std::ifstream fileStream(filename, std::ios::binary);

			// Try opening the file
			if (!fileStream.is_open()) {
				throw std::runtime_error("Couldn't open file!");
			}

			// Validate header and version ("VOX " version 200)
			char header[4];
			int version = 0;

			if (!fileStream.read(header, 4) || std::string(header, 4) != "VOX ") {
				throw std::runtime_error("Invalid file format!");
			}
			if (!fileStream.read(reinterpret_cast<char*>(&version), 4) || version != 200) {
				throw std::runtime_error("Unsupported version!");
			}

			// Main chunk
			VoxChunk main = readChunk(fileStream);

			if (std::string(main.id, 4) != "MAIN") {
				throw std::runtime_error("Missing MAIN chunk!");
			}
			if (main.childSize <= 0) {
				throw std::runtime_error("MAIN chunk is empty!");
			}

			// Child chunks
			main.children = readChildren(fileStream, main.childSize);

			// Get first SIZE and XYZI chunks
			const VoxChunk* sizeChunk = nullptr;
			const VoxChunk* voxelsChunk = nullptr;

			for (const VoxChunk& chunk : main.children) {
				if (sizeChunk && voxelsChunk) break;

				if (!sizeChunk && std::string(chunk.id, 4) == "SIZE") {
					sizeChunk = &chunk;
				}
				else if (!voxelsChunk && std::string(chunk.id, 4) == "XYZI") {
					voxelsChunk = &chunk;
				}
			}

			if (!sizeChunk) throw std::runtime_error("Missing SIZE chunk!");
			if (!voxelsChunk) throw std::runtime_error("Missing XYZI chunk!");

			// Parse size
			VoxSize size = parseChunkSize(*sizeChunk);
			if (size.x <= 0 || size.y <= 0 || size.z <= 0) throw std::runtime_error("Invalid model size!");

			// Parse voxels
			VoxVoxels chunkVoxels = parseChunkVoxels(*voxelsChunk);
			if (chunkVoxels.numVoxels <= 0) throw std::runtime_error("Model has no voxels!");

			// Set model dimensions
			model.dimensions = { static_cast<uint8_t>(size.x), static_cast<uint8_t>(size.z), static_cast<uint8_t>(size.y) };

			// Set model voxels with converted raw voxels
			// (Z and Y are swapped since up is Z in VOX)
			model.voxels.resize(size.x * size.y * size.z);
			for (RawVoxel& raw : chunkVoxels.voxels) {
				model.voxels[raw.x + raw.z * size.x + raw.y * size.x * size.z] = matchVoxelType(raw.colorIndex);
			}

			// Cleanup
			fileStream.close();
			return true;
		}
		catch (const std::exception& error) {
			std::cerr << "Vox Parser: Error loading model. " << error.what() << std::endl;
			return false;
		}
	}
}
