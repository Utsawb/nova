#version 430

in vec2 TexCoord;

uniform sampler2D fontTexture;
uniform vec3 textColor;

out vec4 fragColor;

void main()
{
    float alpha = texture(fontTexture, TexCoord).r;
    fragColor = vec4(textColor, alpha);
}

