#version 400 core
out vec4 FragColor;

in vec3 normal;
in vec2 coords;
in vec3 cubeMapCoords;

uniform samplerCube cubeMap;

uniform float roughness;
uniform int mipLevels;
uniform float exposure;

void main()
{	
	float offset = roughness * float(mipLevels);
	//vec3 color = texture(cubeMap, normal).rgb;
	vec4 color = textureLod(cubeMap, normal, offset);
	//vec3 color = texture(textu, uv).rgb;
	//FragColor = vec4(color, 1.0);
	FragColor = color * exposure;
} 