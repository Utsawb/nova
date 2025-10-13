#version 430

uniform mat4 P;
uniform mat4 MV;
uniform mat4 MV_it;

layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aNor;
layout(location = 2) in vec2 aTexCoordinate; // Texture coordiante

out vec3 vPos;
out vec3 vNor;

out vec2 texCoordinate;

void main()
{
	gl_Position = P * MV * aPos;

	vPos = (MV * aPos).xyz;
	vNor = normalize((MV_it * vec4(aNor, 0.0)).xyz);

	texCoordinate = aTexCoordinate;


}
