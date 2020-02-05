#version 330 core
layout(location = 0) out vec3 color;

in vec3 normal;

uniform vec2 invAtan;
uniform sampler2D envMap;

int numOfPoints = 10;

uint bitfieldReverse(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u)|((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u)|((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u)|((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u)|((bits & 0xFF00FF00u) >> 8u);
	return bits;
}

vec2 computeHammersleyVector(uint i) 
{
	return vec2(float(i)/float(numOfPoints), float(bitfieldReverse(i)) / pow(2, 32));
}


void main()
{	

	
	for(int u = 1; u <= numOfPoints; ++u) {
		vec2 randVec = computeHammersleyVector(uint(u));
	}

	vec3 norm = normalize(normal);

	//typical transformation into sphere UV coordinates
	vec2 uv = vec2(atan(norm.z, norm.x), asin(norm.y));
    uv *= invAtan;
    uv += 0.5; 

	color = texture(envMap, uv).rgb;
	
} 