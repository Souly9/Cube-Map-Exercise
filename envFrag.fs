#version 330 core
out vec4 FragColor;

in vec3 normal;
in vec2 coords;
in vec3 cubeMapCoords;

uniform samplerCube cubeMap;
uniform sampler2D textu;
uniform vec2 invAtan;

void main()
{	

	vec3 norm = normalize(normal);

	//typical transformation into sphere UV coordinates
	vec2 uv = vec2(atan(norm.z, norm.x), asin(norm.y));
    uv *= invAtan;
    uv += 0.5; 
	
	//vec3 color = texture(cubeMap, normal).rgb;
	vec3 color = texture(textu, coords).rgb;
	FragColor = vec4(color, 1.0);
} 