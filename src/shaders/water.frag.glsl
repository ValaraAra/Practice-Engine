#version 460 core
out vec4 FragColor;

in vec4 Albedo;

void main(){
	FragColor = Albedo;
}