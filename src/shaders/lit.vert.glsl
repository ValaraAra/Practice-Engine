#version 460 core
layout (location = 0) in uint packedVertex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2DArray textureArray;
uniform mat3 normal;

out vec3 FragPos;
out vec3 Normal;
out vec3 LightPos;

const uint POS_BITS = 6;
const uint POS_Y_BITS = 8;
const uint FACE_BITS = 3;
const uint TEX_BITS = 9;

const uint X_SHIFT = 0;
const uint Y_SHIFT = POS_BITS;
const uint Z_SHIFT = Y_SHIFT + POS_Y_BITS;
const uint FACE_SHIFT = Z_SHIFT + POS_BITS;
const uint TEX_SHIFT = FACE_SHIFT + FACE_BITS;

const uint POSITION_MASK = (1 << POS_BITS) - 1;
const uint POSITION_Y_MASK = (1 << POS_Y_BITS) - 1;
const uint FACE_MASK = (1 << FACE_BITS) - 1;
const uint TEX_MASK = (1 << TEX_BITS) - 1;

out vec3 vertexColor;

void main()
{
	// Unpacking
	vec3 aPos;
	aPos.x = float((packedVertex >> X_SHIFT) & POSITION_MASK);
	aPos.y = float((packedVertex >> Y_SHIFT) & POSITION_Y_MASK);
	aPos.z = float((packedVertex >> Z_SHIFT) & POSITION_MASK);

	uint face = ((packedVertex >> FACE_SHIFT) & FACE_MASK);
	vec3 faceNormals[6] = vec3[6](
		vec3(1, 0, 0),
		vec3(-1, 0, 0),
		vec3(0, 1, 0),
		vec3(0, -1, 0),
		vec3(0, 0, 1),
		vec3(0, 0, -1)
	);
	vec3 aNorm = faceNormals[face];

	uint texID = ((packedVertex >> TEX_SHIFT) & TEX_MASK);

	// Processing
	gl_Position = projection * view * model * vec4(aPos, 1.0);

	FragPos = vec3(view * model * vec4(aPos, 1.0));
	Normal = normal * aNorm;

	vec3 texCoords = vec3(0, 0, float(texID));
	vertexColor = texture(textureArray, texCoords).rgb;
}