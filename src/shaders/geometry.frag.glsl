#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;

in vec3 FragPos;
in vec3 Normal;
in vec3 VertexColor;

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedo = VertexColor;
}