#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct DirectLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct SpotLight {
	vec3 position;
	vec3 direction;

	float cutOff;
	float outerCutOff;
  
	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

#define NUM_POINT_LIGHTS 2
#define NUM_SPOT_LIGHTS 1

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;

uniform Material material;
uniform DirectLight directLight;
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform SpotLight spotLights[NUM_SPOT_LIGHTS];

vec3 calcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, float ao);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos, float ao);
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 fragPos, float ao);

void main(){
	// From gbuffer
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 VertexColor = texture(gAlbedo, TexCoords).rgb;
	float AO = texture(ssao, TexCoords).r;

	// Setup
	vec3 viewDir = normalize(-FragPos);
	vec3 result = vec3(0.0);

	// Directional Light
	result += calcDirectLight(directLight, Normal, viewDir, AO);

	// Point Lights
	for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
		result += calcPointLight(pointLights[i], Normal, viewDir, FragPos, AO);
	}

	// Spot Lights
	for (int i = 0; i < NUM_SPOT_LIGHTS; i++) {
		result += calcSpotLight(spotLights[i], Normal, viewDir, FragPos, AO);
	}

	vec3 linearVertexColor = pow(VertexColor, vec3(2.2));
	result = result * linearVertexColor;
	result = pow(result, vec3(1.0/2.2)); // Gamma correction
	FragColor = vec4(result, 1.0);
}

vec3 calcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, float ao) {
	vec3 lightDir = normalize(-light.direction);

	// Ambient
	vec3 ambient = light.ambient * material.ambient * ao;

	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse * (diff * material.diffuse);

	// Specular
	vec3 specular = vec3(0.0);
	if (diff != 0.0)
	{
		vec3 halfwayVec = normalize(lightDir + viewDir);
		float spec = pow(max(dot(normal, halfwayVec), 0.0), material.shininess);
		specular = light.specular * (spec * material.specular);
	}

	return (ambient + diffuse + specular);
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos, float ao) {
	vec3 lightDir = normalize(light.position - fragPos);

	// Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	// Ambient
	vec3 ambient = light.ambient * material.ambient * ao;
	ambient *= attenuation;

	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse * (diff * material.diffuse);
	diffuse *= attenuation;

	// Specular
	vec3 specular = vec3(0.0);
    if (diff != 0.0)
    {
        vec3 halfwayVec = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayVec), 0.0), material.shininess);
        specular = light.specular * (spec * material.specular);
		specular *= attenuation;
	}

	return (ambient + diffuse + specular);
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 fragPos, float ao) {
	vec3 lightDir = normalize(light.position - fragPos);

	// Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	// Intensity
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

	// Ambient
	vec3 ambient = light.ambient * material.ambient * ao;
	ambient *= attenuation * intensity;

	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse * (diff * material.diffuse);
	diffuse *= attenuation * intensity;

	// Specular
    vec3 specular = vec3(0.0);
    if (diff != 0.0)
    {
        vec3 halfwayVec = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayVec), 0.0), material.shininess);
        specular = light.specular * (spec * material.specular);
		specular *= attenuation * intensity;
	}

	return (ambient + diffuse + specular);
}