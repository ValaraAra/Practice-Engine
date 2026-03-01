#version 460 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D blurInput;
uniform int radius;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(blurInput, 0));
    float result = 0.0;

    for (int x = -radius; x <= radius; x++)
    {
        for (int y = -radius; y <= radius; y++) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(blurInput, TexCoords + offset).r;
        }
    }

	int size = radius * 2 + 1;
    FragColor = result / (size * size);
}