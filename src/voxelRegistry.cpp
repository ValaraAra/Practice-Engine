#include "voxelRegistry.h"

std::vector<VoxelData> VoxelRegistry::voxelList{
	EmptyVoxel,
	ErrorVoxel
};

std::unordered_map<std::string, uint16_t> VoxelRegistry::voxelMap{
	{ EmptyVoxel.stringID, EmptyVoxelID },
	{ ErrorVoxel.stringID, ErrorVoxelID }
};

uint16_t VoxelRegistry::Register(const VoxelData& data) {
	auto it = voxelMap.find(data.stringID);
	if (it != voxelMap.end()) {
		return it->second;
	}

	const uint16_t id = static_cast<uint16_t>(voxelList.size());

	voxelList.push_back(data);
	voxelMap[data.stringID] = id;
	return id;
};

uint16_t VoxelRegistry::GetID(const std::string& stringID) {
	auto it = voxelMap.find(stringID);
	if (it != voxelMap.end()) {
		return it->second;
	}

	return 1u;
};

const VoxelData& VoxelRegistry::GetData(const std::string& stringID) {
	auto it = voxelMap.find(stringID);
	if (it != voxelMap.end()) {
		return voxelList[it->second];
	}

	return ErrorVoxel;
};