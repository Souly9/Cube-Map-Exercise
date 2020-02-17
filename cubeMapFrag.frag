#version 400 core
layout(location = 0) out vec3 color;

in vec3 cubeMapCoords;

uniform sampler2D envMap;
uniform float specular;
uniform float PI;
uniform float numOfPoints;

/*Fragment Shader takes in an environment Map in longitute latitude form and filters it based on the fragment position
* Process is based on the concept of Importance Sampling with random Hammersley Points being generated to compute the light at the points around the fragment
* Each miplevel is filtered to appear rougher based on the specular exponent and decreasing resolution
*/


//compute Hammersley Points in spherical coordinates
vec2 computeHammersleyPoint(uint i) 
{
	//temporary Hammersley point 
	vec2 H = vec2(float(i) / numOfPoints, bitfieldReverse(i) / float(pow(2, 32)));
	
	//the final vector in spherical coordinates
	//specular exponent is considered due to the importance sampling
	//divided into two steps to make it more readable
	vec2 sphereC;
	sphereC.x = acos(pow(H.y, 1.0/(specular + 1)));
	sphereC.y = 2 * PI * H.x;

	return sphereC;
}

//Transform a vector into the carthesian coordinate system
vec3 computeKarthesian(vec2 hVector) {
	
	vec3 karthesianVec;
	float sineTheta = sin(hVector.x);

	karthesianVec.x = sineTheta * cos(hVector.y);
	karthesianVec.y = sineTheta * sin(hVector.y);
	karthesianVec.z = cos(hVector.x);

	return karthesianVec;
}

//Main function to generate a sample vector through the Hammersley sequence
vec3 randomSample(uint i, vec3 normal) {

	vec3 randPoints;
	vec3 sampleVec;
	vec3 norm = normalize(normal);
    vec3 tangent   = vec3(0); 
    vec3 bitangent = cross(norm, tangent);

	randPoints.xy = computeHammersleyPoint(i);
	randPoints = computeKarthesian(randPoints.xy);
	sampleVec = (randPoints.x * tangent) + (randPoints.y * bitangent) + (randPoints.z * norm);
	
	return sampleVec;
}

vec3 sampleEnvMap(vec3 vector) {
	//Let's transform the points into vectors 
	//Done through transforming them to spehrical coordinates
	vec3 norm = normalize(vector);

	//transformation of the cube Map Coordinates into spherical coordinates to sample the environemnt Map
	vec2 uv;
	uv.x = (atan(norm.x, norm.z)) / (2 * PI) + 0.5;
	uv.y = (norm.y) * 0.5 + 0.5;
	return texture(envMap, uv).rgb;
}

//Main function to sample the environment map through Importance Sampling and filter it accordint to miplevel
vec3 filterMap(vec3 normal) {
	float sum = 0.0;
	vec3 result = vec3(0.0);

	for(uint i = 0u; i < numOfPoints; ++i) 
	{
		vec3 L = randomSample(i, normal);
		float NoL = max(dot(normal, L), 0);

		float pNoL = pow(NoL, specular);
		
		result += sampleEnvMap(L) * pNoL;
		sum += pNoL;
		
	}

	return result/sum;
}

void main()
{	
	color = filterMap(cubeMapCoords);
} 