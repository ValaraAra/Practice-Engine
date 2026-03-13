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
	vec4 Albedo = texture(gAlbedo, TexCoords);
	float AO = texture(ssao, TexCoords).r;
	
	// Linearize
	vec3 linear = pow(Albedo.rgb, vec3(2.2));

	// Apply AO
	linear *= AO;

	// Gamma correction
	linear = pow(linear, vec3(1.0/2.2));
	FragColor = vec4(linear, Albedo.a);
}