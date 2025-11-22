#version 460 core
out vec4 color;

uniform float iTime;

void main(){
	float t = iTime + 10.0;

	float r = abs(sin(t * 0.3));
	float g = abs(sin(t * 0.6));
	float b = abs(sin(t * 1.0));

	color = vec4(r, g, b, 1.0);
}