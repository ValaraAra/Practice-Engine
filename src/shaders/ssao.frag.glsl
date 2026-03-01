#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform int kernelSize;
uniform float radius;
uniform float bias;

uniform vec3 samples[64];
uniform mat4 projection;
uniform vec2 iResolution;

void main(){
	vec2 noiseScale = iResolution / 4.0;
	
	// Get data from gbuffer
	vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = texture(gNormal, TexCoords).rgb;
	vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;

	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// Accumulate occlusion
	float occlusion = 0.0;

	for (int i = 0; i < kernelSize; i++)
	{
		// Get sample position (tan to view space)
		vec3 samplePos = TBN * samples[i];
		samplePos = fragPos + samplePos * radius;

		// Get offset (view to clip space, perspective divide, transform to [0,1])
		vec4 offset = vec4(samplePos, 1.0);
		offset = projection * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5 + 0.5;

		// Get sample depth
		float sampleDepth = texture(gPosition, offset.xy).z;

		// Check range & accumulate
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	// Normalize occlusion contribution
	occlusion = 1.0 - (occlusion / kernelSize);

	// Output results
	FragColor = vec4(vec3(occlusion), 1.0);
}
