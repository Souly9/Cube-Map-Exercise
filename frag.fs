#version 330 core
out vec4 FragColor;

in vec3 normal;
in vec3 worldPos;

uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{	
	float ambientStrength = 0.1;
	float specularStrength = 0.5;
	int shininess = 32;

	vec3 norm = normalize(normal);

	vec3 lightDir = normalize(lightPos - worldPos);
	vec3 viewDir = normalize(viewPos - worldPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	float diff = max(dot(norm, lightDir), 0.0);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 specular = specularStrength * spec * lightColor;
	vec3 diffuse = diff * lightColor;

	vec3 ambient = ambientStrength * lightColor;

	vec3 result = (ambient + diffuse + specular) * objectColor;

	 FragColor = vec4(result, 1.0f);
} 