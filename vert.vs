#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 texCoords;

out vec3 normal;
out vec2 coords;
out vec3 cubeMapCoords;

uniform mat4 transMat;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;


void main()
{
	normal = aNormal;
	cubeMapCoords = aPos;
	gl_Position =  projMatrix * viewMatrix * transMat * vec4(aPos, 1);
	
	coords = texCoords;
	
}