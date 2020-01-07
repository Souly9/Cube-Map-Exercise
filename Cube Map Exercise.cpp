#include "pch.h"
#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include "Shader.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <xlocmon>
#include "stb_image.h"

//invAtan is a vec2 consisting of the parameters to clamp the sphere coordinates into the UV range
//which are computed in the Fragment Shader 
//x-coord: 2*PI approximation
//y-coord: PI approximation
#define invAtan glm::vec2(0.1591, 0.3183)
#define PI 3.14159265358979323846
#define screenWidth 800
#define screenHeight 600

//lighting variables
glm::vec3 lightPos(0.0f, 0.0f, 1.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

//later used to manipulate rotation from processInput method for rendering, hence the global variable
glm::mat4 transMatrix = translate(glm::mat4(1.0f), glm::vec3(0, 0, -4.0));

//helper variables to control rotation with arrow keys
bool left = false, up = false;
float rotateX = 0, rotateY = 0;


void processInput(GLFWwindow* window);

//method to create Sphere Coordinates to draw the Sphere! Check definition for more details
void createSphereCoordinates(float radius, float sectors, float stacks, std::vector<unsigned int> &EBO, std::vector<float>& buffer);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "CubeMap", nullptr, nullptr);

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, screenWidth, screenHeight);

	glEnable(GL_DEPTH_TEST);

	//Tell OpenGL we want to call our previously define resize function on resize
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	Shader ourShader("vert.vs", "envFrag.fs");
	Shader cubeMapShader("cubeMapVert.vs", "cubeMapFrag.fs");
	
	//sample the environment map

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	
	float *data = stbi_loadf("cedar_bridge_1k1.hdr",
		&width, &height, &nrChannels, 0);
	if (!data)
		std::cout << "Image not loaded correctly" << std::endl;
	
	//creating a texture out of the hdr image
	unsigned int envMap;
	glGenTextures(1, &envMap);
	glBindTexture(GL_TEXTURE_2D, envMap);

	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);

	unsigned int cubeMapBuffer;
	glGenFramebuffers(1, &cubeMapBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapBuffer);

	unsigned int cubeMap;
	glGenTextures(1, &cubeMap);
	glBindTexture(GL_TEXTURE_2D, cubeMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cubeMap, 0);
	
	if (glCheckFramebufferStatus((GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE))
		std::cout << "yeeeeees";

	//Now that we have our own Buffer for the Cubemap rendering we can create the needed geometry
	float squareCoordinates[] = {
		-1.0f,  1.0f,  
		-1.0f, -1.0f,  
		 1.0f, -1.0f,
		 1.0f, 1.0f
	};
	//indices for drawing the square
	unsigned int squareIndices[] = {
	0, 1, 2,
	0, 1, 3
	};

	//usual buffer creation and binding
	unsigned int squareBuffer, squareIndexBuffer, squareVAO;
	glGenVertexArrays(1, &squareVAO);
	glGenBuffers(1, &squareBuffer);
	glGenBuffers(1, &squareIndexBuffer);

	glBindVertexArray(squareVAO);

	glBindBuffer(GL_ARRAY_BUFFER, squareBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(squareCoordinates), squareCoordinates, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, squareIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float), static_cast<void*>(nullptr));
	glEnableVertexAttribArray(0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	//get our relevant coordinates and begin filling up the buffers for the sphere model
	std::vector<unsigned int> indices;
	std::vector<float> vertices;
	createSphereCoordinates(2.0, 144, 72, indices, vertices);

	unsigned int buffer, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &buffer);
	glGenBuffers(1, &EBO);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);;
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void*>(nullptr));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glClearColor(1.0f, 0.3f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cubeMapShader.use();

		glBindFramebuffer(GL_FRAMEBUFFER, cubeMapBuffer);
		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(squareVAO);
		glBindTexture(GL_TEXTURE_2D, cubeMap);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ourShader.use();

		ourShader.setMat4("transMat", transMatrix);
		ourShader.setMat4("projMatrix", glm::perspective(glm::radians(90.0f),
														 static_cast<float>(screenWidth) / static_cast<float>(
															 screenHeight), 0.1f, 100.0f));
		ourShader.setMat4("viewMatrix", glm::mat4(1.0f));
		glBindTexture(GL_TEXTURE_2D, cubeMap);
		glBindVertexArray(VAO);
		

		/*glUniform3fv(glGetUniformLocation(ourShader.ID, "lightColor"), 1, value_ptr(lightColor));
		glUniform3fv(glGetUniformLocation(ourShader.ID, "objectColor"), 1, value_ptr(glm::vec3(1.0f, 0.5f, 0.2f)));
		glUniform3fv(glGetUniformLocation(ourShader.ID, "lightPos"), 1, value_ptr(lightPos));
		glUniform3fv(glGetUniformLocation(ourShader.ID, "viewPos"), 1, value_ptr(glm::vec3(0, 0, 0)));*/
		glUniform2fv(glGetUniformLocation(ourShader.ID, "invAtan"), 1, value_ptr(invAtan));

		
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &buffer);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		if (left)
			rotateY = 0;
		left = false;
		rotateY += 0.01f;
		transMatrix = glm::mat4(rotate(transMatrix, glm::radians(rotateY), glm::vec3(0, 1, 0)));
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		if (up)
			rotateX = 0;
		up = false;
		rotateX -= 0.01f;
		transMatrix = glm::mat4(rotate(transMatrix, glm::radians(rotateX), glm::vec3(1, 0, 0)));
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		if (!up)
			rotateX = 0;
		up = true;
		rotateX += 0.01f;
		transMatrix = glm::mat4(rotate(transMatrix, glm::radians(rotateX), glm::vec3(1, 0, 0)));
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		if (!left)
			rotateY = 0;
		left = true;
		rotateY -= 0.01f;
		transMatrix = glm::mat4(rotate(transMatrix, glm::radians(rotateY), glm::vec3(0, 1, 0)));
	}
}


/*	Method takes the sphere radius as first input
 *	Calculation happens based on the sector and stacks principle where the Sphere is divided into tinier spaces
 *	Then we can approximate the vertices of those spaces
 *
 *	radius: the radius of the sphere
 *	sectors: number of pieces along the x-axis
 *	stacks: number of pieces along the y-axis
 *	EBO: the EBO in which the indices should be written into to draw the sphere later
 *	buffer: the VAO into which the coordinates should be written
 *	
 *	Writes the indices into the specified EBO and the coordinates, normals as well as texture coordinates into the buffer
 */
void createSphereCoordinates(float radius, float sectors, float stacks, std::vector<unsigned int> &EBO, std::vector<float>& buffer)
{
	std::vector<float> texCoords;

	float x, y, z;

	//helper variables for normal calculation
	float nx, ny, nz;

	float s, t;

	//declared for the normal calculation:
	//multiplying a given coordinate by the length Inverse results 
	float lengthInv = 1.0f / radius;


	float sectorAngle, stackAngle;

	//some helper values
	float cosStack, sinStack;

	//compute coordinates, normals and texCoords in this order, using the known formulas to compute each point on a sphere
	for (float i = 0; i <= stacks; ++i)
	{
		stackAngle = (PI / 2) - PI * (i / stacks);
		cosStack = cosf(stackAngle);
		z = radius * sinf(stackAngle);

		for (float j = 0; j <= sectors; ++j)
		{
			sectorAngle = 2 * PI * (j / sectors);

			x = (radius * cosStack) * cosf(sectorAngle);
			y = (radius * cosStack) * sinf(sectorAngle);
			buffer.push_back(x);
			buffer.push_back(y);
			buffer.push_back(z);

			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			buffer.push_back(nx);
			buffer.push_back(ny);
			buffer.push_back(nz);

			s = static_cast<float>(j) / sectors;
			t = static_cast<float>(i) / stacks;
			buffer.push_back(s);
			buffer.push_back(t);
		}
	}

	int s1, s2;
	// logic to create the index buffer
	for (int i = 0; i < stacks; ++i)
	{
		// determine our current stack and next stack
		s1 = i * (sectors + 1);
		s2 = s1 + sectors + 1;

		/* iterate through the two stacks, adding two triangles each iteration for every stack except the top and bottom one
		 * since we have to draw counter clockwise in order to make the face appear outside:
		 * k1 -> k2 -> k1+1 gives us a triangle with the center at the bottom
		 * s1+1 -> s2 -> s2+1 creates a triangle with the center at the top
		 */
		for (int j = 0; j < sectors; ++j, ++s1, ++s2)
		{
			//we need 2 vertices from first sector and 1 from next sector
			if (i != 0)
			{
				EBO.push_back(s1);
				EBO.push_back(s2);
				EBO.push_back(s1+1);
			}

			//we need 1 vertices from first sector and 2 from next sector
			if (i != (stacks - 1))
			{
				EBO.push_back(s1+1);
				EBO.push_back(s2);
				EBO.push_back(s2 + 1);
			}
		}
	}
}
