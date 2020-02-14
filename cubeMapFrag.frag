#version 400 core
layout(location = 0) out vec3 color;

in vec3 cubeMapCoords;

uniform sampler2D envMap;
uniform float specular;
uniform float PI;
uniform float numOfPoints;

//compute Hammersley Points in spherical coordinates
vec2 computeHammersleyPoint(uint i) 
{
	vec2 H = vec2(float(i) / numOfPoints, bitfieldReverse(i) / float(pow(2, 32)));
	
	vec2 sphereC;

	//H.y = H.x / (2*PI);
	//H.x = acos(pow(H.y, 1.0/(specular+1)));
	
	sphereC.x = acos(pow(H.y, 1.0/(specular + 1)));
	sphereC.y = H.x / (2*PI);
	
	sphereC = normalize(sphereC);
	
	return sphereC;
}

vec3 computeKarthesian(vec2 hVector) {
	vec3 karthesianVec;
	
	float sineTheta = sin(hVector.x);

	karthesianVec.x = sineTheta * cos(hVector.y);
	karthesianVec.y = sineTheta * sin(hVector.y);
	karthesianVec.z = cos(hVector.x);

	return normalize(karthesianVec);
}

vec3 computePerpendicular(vec3 vector) {
	return vec3(vector.y, -vector.x, vector.z);
}

vec3 randomSample(uint i, vec3 normal) {

	vec3 randPoints;
	vec3 sampleVec;

	vec3 norm = normalize(normal);

	vec3 up        = abs(norm.z) <1 ?  vec3(0.0, 1.0, 1.0) : vec3(1.0, 0.0, 0.0);
	
    vec3 tangent   = normalize(cross(up, norm)); // normalize(up);
    vec3 bitangent = cross(norm, tangent);

	randPoints.xy = computeHammersleyPoint(i);
	randPoints = computeKarthesian(randPoints.xy);
	randPoints = normalize(randPoints);
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