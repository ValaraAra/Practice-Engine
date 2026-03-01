#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;

void main(){
	// From gbuffer
	vec3 albedo = texture(gAlbedo, TexCoords).rgb;
	float AO = texture(ssao, TexCoords).r;
	
	// Linearize
	albedo = pow(albedo, vec3(2.2));

	// Apply AO
	albedo *= AO;

	// Gamma correction
	albedo = pow(albedo, vec3(1.0/2.2));
	FragColor = vec4(albedo, 1.0);
}