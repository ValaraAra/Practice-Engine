#version 460 core
layout (location = 0) in vec3 localPos;
layout (location = 1) in uint packedFace;

out vec4 Albedo;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2DArray textureArray;

const uint POS_BITS = 5;
const uint POS_Y_BITS = 7;
const uint FACE_BITS = 3;
const uint TEX_BITS = 12;

const uint X_SHIFT = 0;
const uint Y_SHIFT = POS_BITS;
const uint Z_SHIFT = Y_SHIFT + POS_Y_BITS;
const uint FACE_SHIFT = Z_SHIFT + POS_BITS;
const uint TEX_SHIFT = FACE_SHIFT + FACE_BITS;

const uint POSITION_MASK = (1 << POS_BITS) - 1;
const uint POSITION_Y_MASK = (1 << POS_Y_BITS) - 1;
const uint FACE_MASK = (1 << FACE_BITS) - 1;
const uint TEX_MASK = (1 << TEX_BITS) - 1;

const vec3 faceNormals[6] = vec3[6](
	vec3(1, 0, 0),
	vec3(-1, 0, 0),
	vec3(0, 1, 0),
	vec3(0, -1, 0),
	vec3(0, 0, 1),
	vec3(0, 0, -1)
);

const mat3 faceRotations[6] = mat3[6](
	mat3( 0, 0,-1,   0, 1, 0,   1, 0, 0),
	mat3( 0, 0, 1,   0, 1, 0,  -1, 0, 0),
	mat3( 1, 0, 0,   0, 0,-1,   0, 1, 0),
	mat3( 1, 0, 0,   0, 0, 1,   0,-1, 0),
	mat3( 1, 0, 0,   0, 1, 0,   0, 0, 1),
	mat3(-1, 0, 0,   0, 1, 0,   0, 0, -1)
);

void main()
{
	// Normal
	uint face = ((packedFace >> FACE_SHIFT) & FACE_MASK);

	// Color
	uint texID = ((packedFace >> TEX_SHIFT) & TEX_MASK);
	vec3 texCoords = vec3(0, 0, float(texID));
	Albedo = texture(textureArray, texCoords);
	
	// Position
	vec3 chunkPos;
	chunkPos.x = float((packedFace >> X_SHIFT) & POSITION_MASK);
	chunkPos.y = float((packedFace >> Y_SHIFT) & POSITION_Y_MASK);
	chunkPos.z = float((packedFace >> Z_SHIFT) & POSITION_MASK);

	vec3 faceOffset = faceNormals[face] * 0.5;
	vec3 worldLocalPos = faceRotations[face] * localPos + faceOffset + chunkPos;
	
	gl_Position = projection * view * model * vec4(worldLocalPos, 1.0);
}