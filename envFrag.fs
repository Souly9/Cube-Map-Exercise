#version 400 core
out vec4 FragColor;

in vec3 cubeMapCoords;

uniform samplerCube cubeMap;
uniform float roughness;
uniform int mipLevels;
uniform float exposure;

void main()
{	
	//offset to switch between mipmaps as each of them is filtered to be rougher than the previous one
	float offset = roughness * float(mipLevels);

	FragColor = textureLod(cubeMap, cubeMapCoords, offset) * exposure;
} 