#version 400 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 cubeMapCoords;

uniform mat4 transMat;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;

void main()
{
	cubeMapCoords = aNormal;
	gl_Position =  projMatrix * viewMatrix * transMat * vec4(aPos, 1);	
}