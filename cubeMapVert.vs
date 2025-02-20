#version 400 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 cubeMapCoords;

void main()
{
	//no need for transformations since the coordinates are between 0.0 and 1.0
	cubeMapCoords = aNormal;
	gl_Position = vec4(aPos, 1);
}