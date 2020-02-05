#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 normal;

uniform mat4 transMat;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main()
{
	//no need for transformations since the rendered quad is filling the whole screen
	normal = aNormal;
	gl_Position = vec4(aPos, 1);
	
}