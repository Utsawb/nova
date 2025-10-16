#version 430

in vec3 vPos;
in vec3 vNor;

in vec2 texCoordinate;
uniform sampler2D inTexture;

out vec4 fragColor;

void main()
{	
    //fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    fragColor = texture(inTexture, texCoordinate);
}
