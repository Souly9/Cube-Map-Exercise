#include "pch.h"
#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include "Shader.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <xlocmon>
#include "stb_image.h"


//invAtan is a vec2 consisting of the parameters to clamp the sphere coordinates into the UV range
//which are computed in the Fragment Shader 
//x-coord: 2*PI approximation
//y-coord: PI approximation
#define numOfPoints 2048
#define PI 3.14159265358979323846
#define screenWidth 800
#define screenHeight 600
#define cubeMapWidth 512
#define cubeMapHeight 512

float mipmaps = 8.0f;

float totalXRotation = 0.0f;
glm::quat orientationQuat = glm::quat(1, 0, 0, 0);

//variables to control the texture
double roughness = 0.0f;
double exposure = 1.0;

//lighting variables
glm::vec3 lightPos(0.0f, 0.0f, 1.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

//later used to manipulate rotation from processInput method for rendering, hence the global variable
glm::mat4 Model = glm::mat4(1.0f);
glm::mat4 view = lookAt(glm::vec3(0.0f, 0.0f, 1.5f),
                        glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f));

//helper variables to control rotation by mouse
float lastX, lastY;
bool unSet = true;
bool mouseButtonPressed = false;

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
//method to create Sphere Coordinates to draw the Sphere! Check definition for more details
void createSphereCoordinates(float radius, float sectors, float stacks, std::vector<unsigned int>& EBO,
                             std::vector<float>& buffer);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "CubeMap", nullptr, nullptr);

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, screenWidth, screenHeight);

	//Set callback functions
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, mouse_move_callback);

	//initialize the Shaders for the Offscreen-Renderpass and final image
	Shader cubeMapShader("cubeMapVert.vs", "cubeMapFrag.frag");
	Shader ourShader("vert.vs", "envFrag.fs");

	//create and read the environment map texture
	//-------------------------------------------------------------------------
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);

	float* data = stbi_loadf("cedar_bridge_1k1.hdr",
	                         &width, &height, &nrChannels, 0);

	if (!data)
		std::cout << "Image not loaded correctly" << std::endl;

	unsigned int envMap;
	glGenTextures(1, &envMap);
	glBindTexture(GL_TEXTURE_2D, envMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);

	//allocate framebuffer for the cubemap generation
	//-------------------------------------------------------------------------
	unsigned int cubeMapBuffer;
	glGenFramebuffers(1, &cubeMapBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapBuffer);

	unsigned int bufferTexture;
	glGenTextures(1, &bufferTexture);
	glBindTexture(GL_TEXTURE_2D, bufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, cubeMapWidth, cubeMapHeight, 0, GL_RGB, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bufferTexture, 0);

	// Set the draw buffers, only one per pass is needed in this case since we only want to render one cubemap face
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	//Now that we have our Buffer for the Cubemap rendering we can create the needed geometry
	//-------------------------------------------------------------------------

	//square that covers the whole screen for the cubemap face generation
	float squareCoordinates[] = {
		//vertex coordinates   //normals
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
	};

	//cubemap coordinates for the front and back points in 3d space
	/*orientation analog for front and back, numbers are the indices:
	 * 3---0
	 * |   |
	 * |   |
	 * 2---1
	 */
	float cubeNormals[] = {
		//front
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		//back
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f
	};

	//indices to assign the normals to the appropriate faces 
	short cubeIndices[]
	{
		//right
		5, 4, 0, 1,
		//left
		2, 3, 7, 6,

		//top
		0, 4, 7, 3,
		//bottom
		5, 1, 2, 6,

		//back
		1, 0, 3, 2,
		//front
		6, 7, 4, 5
	};

	//indices for drawing the square
	unsigned int squareIndices[] = {
		0, 1, 3,
		1, 2, 3
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

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), static_cast<void*>(nullptr));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//offscreen renderpass
	//-------------------------------------------------------------------------

	//create mipmap resolution array
	std::vector<int> mipResolutions;
	mipResolutions.push_back(cubeMapHeight);
	for (int i = 1; i < mipmaps + 1; ++i)
	{
		mipResolutions.push_back(mipResolutions[i - 1] * 0.5);
	}

	cubeMapShader.use();

	//no depth test needed since the square covers the entire screen
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapBuffer);

	//generate the main Cubemap
	unsigned int cubeMapTexture;
	glGenTextures(1, &cubeMapTexture);

	glUniform1f(glGetUniformLocation(cubeMapShader.ID, "numOfPoints"), numOfPoints);
	glUniform1f(glGetUniformLocation(cubeMapShader.ID, "PI"), PI);

	for (int i = 0; i < 6; ++i)
	{
		//loop to change the normals for each face
		//-------------------------------------------------------------------------
		for (int n = 0; n < 4; ++n)
		{
			//offset to iterate over the second batch of coordinates in our vertex buffer
			//every line is 6 indeces big and 3 of them are the screen Coordinates
			short offset = n * 6 + 3;

			//compute the index of the next coordinate as cubeIndices holds all indices for all Cubemap faces
			//every line of indices in cubeIndices is sorted by face index,
			//the index of n-th point of the i-th face (multiplied by 4 since 4 indices are in every row and have to be jumped over
			//This index is then mulitplied by 3 to jump over each pair of 3 coordinates in the actual cubeNormals array for index-times
			short pos = cubeIndices[n + (4 * i)] * 3;

			//since both offset and pos now point to the first coordinate of a coordinate triplet we can simply iterate over it
			squareCoordinates[offset++] = cubeNormals[pos++];
			squareCoordinates[offset++] = cubeNormals[pos++];
			squareCoordinates[offset] = cubeNormals[pos];
		}

		glBindVertexArray(squareVAO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(squareCoordinates), squareCoordinates, GL_STATIC_DRAW);

		//loop to create the MIP levels manually
		//9 since the base texture is 512 pixel (2^9)
		//-------------------------------------------------------------------------
		for (int mipLevel = 0; mipLevel < 9; ++mipLevel)
		{
			int tempCubeMapWidth = mipResolutions[mipLevel];
			int tempCubeMapHeight = mipResolutions[mipLevel];

			//resize the viewport to fit the mipmap resolution
			glViewport(0, 0, tempCubeMapWidth, tempCubeMapHeight);

			glBindTexture(GL_TEXTURE_2D, envMap);

			//compute the specular exponent, the higher the mip level the lower the value should be
			float exponent = 1.0f - (mipLevel / mipmaps);
			float specular = pow(2, 15 * exponent);

			glUniform1f(glGetUniformLocation(ourShader.ID, "specular"), specular);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

			//bind the cubemap after the offscreen rendering pass
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			//compute texture dimensions and read it into the buffer
			float* texBuffer = new float[tempCubeMapWidth * tempCubeMapHeight * 3];

			glReadPixels(0, 0, tempCubeMapWidth, tempCubeMapHeight, GL_RGB, GL_FLOAT, texBuffer);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			             mipLevel,
			             GL_RGB32F,
			             tempCubeMapWidth, tempCubeMapHeight, 0,
			             GL_RGB, GL_FLOAT, texBuffer);
		}
	}

	//set different parameters for filtering
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, mipmaps);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_SEAMLESS, 1);

	//Bind our main framebuffer to actually prepare the final scene
	//-------------------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//get our relevant coordinates and begin filling up the buffers for the sphere model
	std::vector<unsigned int> indices;
	std::vector<float> vertices;
	createSphereCoordinates(1.0, 144, 72, indices, vertices);


	unsigned int buffer, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &buffer);
	glGenBuffers(1, &EBO);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	//coordinates
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), static_cast<void*>(nullptr));
	//normals
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);


	//Viewport reset
	glViewport(0, 0, screenWidth, screenHeight);
	glEnable(GL_DEPTH_TEST);

	glBindTexture(GL_TEXTURE_2D, 0);

	//main render loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		glClearColor(0.2f, 0.3f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ourShader.use();

		ourShader.setMat4("transMat", Model);
		ourShader.setMat4("projMatrix", glm::perspective(glm::radians(90.0f),
		                                                 static_cast<float>(screenWidth) / static_cast<float>(
			                                                 screenHeight), 0.1f, 100.0f));
		ourShader.setMat4("viewMatrix", view);

		glUniform1f(glGetUniformLocation(ourShader.ID, "roughness"), roughness);
		glUniform1i(glGetUniformLocation(ourShader.ID, "mipLevels"), mipmaps);
		glUniform1f(glGetUniformLocation(ourShader.ID, "exposure"), exposure);

		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		glBindVertexArray(VAO);

		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &squareVAO);
	glDeleteBuffers(1, &buffer);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &squareBuffer);
	glDeleteBuffers(1, &squareIndexBuffer);

	glfwTerminate();
	return 0;
}

//process input to increase roughness and exposure as well as switch between PolygonModes
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		if (roughness < 0.9999)
			roughness += 0.001f;
		std::cout << "Roughness: " << roughness << "   " << "Exposure: " << exposure << std::endl;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		if (exposure < 1.9999)
			exposure += 0.001f;
		std::cout << "Roughness: " << roughness << "   " << "Exposure: " << exposure << std::endl;
	}
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		mouseButtonPressed = true;
		unSet = true;
		return;
	}
	mouseButtonPressed = false;
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (mouseButtonPressed)
	{
		if (unSet)
		{
			lastX = xpos;
			lastY = ypos;
			unSet = false;
		}
		if (xpos < lastX || ypos < lastY || xpos > lastX || ypos > lastY)
		{
			orientationQuat = glm::quat(1, 0, 0, 0);
			Model = Model * toMat4(orientationQuat);
		}
		float xOffset = xpos - lastX;
		float yOffset = ypos - lastY;
		lastX = xpos;
		lastY = ypos;

		const float sensitivity = 0.1f;
		xOffset *= sensitivity;
		yOffset *= sensitivity;

		glm::vec3 yAxis = glm::vec3(0.0, 1.0, 0.0);
		glm::vec3 zAxis = glm::vec3(1.0, 0, 0.0);

		glm::quat yQuat = angleAxis(glm::radians(yOffset), zAxis);
		glm::quat xQuat = angleAxis(glm::radians(xOffset), yAxis);
		orientationQuat *= yQuat * xQuat;
		normalize(orientationQuat);

		Model = toMat4(orientationQuat) * Model;
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
void createSphereCoordinates(float radius, float sectors, float stacks, std::vector<unsigned int>& EBO,
                             std::vector<float>& buffer)
{
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
				EBO.push_back(s1 + 1);
			}

			//we need 1 vertices from first sector and 2 from next sector
			if (i != (stacks - 1))
			{
				EBO.push_back(s1 + 1);
				EBO.push_back(s2);
				EBO.push_back(s2 + 1);
			}
		}
	}
}
