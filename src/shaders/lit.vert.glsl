#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;
layout (location = 2) in vec3 aNorm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;

out vec3 FragPos;
out vec3 Normal;
out vec3 LightPos;

out vec3 vertexColor;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);

	FragPos = vec3(view * model * vec4(aPos, 1.0));
	Normal = mat3(transpose(inverse(view * model))) * aNorm; // Should do this on the CPU and pass it as a uniform
	LightPos = vec3(view * vec4(lightPos, 1.0));

	vertexColor = aCol;
}