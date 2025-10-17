#version 430

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 P;
uniform mat4 MV;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = P * MV * vec4(aPos, 0.0, 1.0);
}

