#version 330 core
out vec4 FragColor;

in vec3 normal;
in vec2 coords;

uniform sampler2D envMap;
uniform vec2 invAtan;

void main()
{	

	vec3 norm = normalize(normal);
	
	//typical transformation into sphere UV coordinates
	vec2 uv = vec2(atan(norm.z, norm.x), asin(norm.y));
    uv *= invAtan;
    uv += 0.5;
	vec4 color = texture(envMap, uv);
	color *= vec4(1.0, 0.0, 0.0, 1.0);
	FragColor = color ;
} 