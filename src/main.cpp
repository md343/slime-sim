#define NUM_PARTICLES 1024*512
#define WORK_GROUP_SIZE 1024
#define WRITELOG 1
#include"glad/glad.h"
#include<GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include<math.h>
#include<iostream>
//#include<GL/glew.h>
#include<stdio.h>
#include<stdlib.h>
#include<glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include<GL/gl.h>
//#include<GL/glext.h>
#include<time.h>
#include<shaderClass.h>
#include<computeClass.h>
#include<VAO.h>
#include<VBO.h>
#include<EBO.h>
#include<SSBO.h>
#include<texture.h>


const unsigned int width = 1024;
const unsigned int height = 1024;

struct agent* setAgents();
void timer();
GLFWwindow* setup();
//void setCells();

GLfloat vertices[] =
{
	-1.0f, -1.0f , 0.0f, 0.0f, 0.0f,
	-1.0f,  1.0f , 0.0f, 0.0f, 1.0f,
	 1.0f,  1.0f , 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f , 0.0f, 1.0f, 0.0f,
};

GLuint indices[] =
{
	0, 2, 1,
	0, 3, 2
};

struct agent
{
    glm::vec4 position;
};

// struct cell
// {
// 	float val = 0;
// };

float prevTime = 0.0;
float coutTime = 0.0;
float crntTime = 0.0;
float timeDiff;
float coutDiff = 10.0;
float frames = 0;

int main()
{
    srand (time(NULL));
	// Initialize GLFW
	// GLFWwindow* window = setup();
	glfwInit();
	std::cout << "STRUCT SIZE: " << sizeof(struct agent)<< std::endl;
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	
	GLFWwindow* window = glfwCreateWindow(width, height, "Compute Shader Test", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGL();

	glViewport(0, 0, width, height);

	VAO VAO1;

	VBO VBO1(vertices, sizeof(vertices));
	EBO EBO1(indices, sizeof(indices));

	VAO1.LinkAttrib(0, 3, GL_FLOAT, 0);
	VAO1.LinkAttrib(1, 2, GL_FLOAT, 3 * sizeof(float));

	VAO1.LinkVertBuffer(VBO1, 5 * sizeof(GLfloat));
	VAO1.LinkElemBuffer(EBO1);

	Texture Tex0 = Texture(width, height);

	SSBO AgentSBO(NUM_PARTICLES, sizeof(struct agent));
	struct agent* points = setAgents();

	//SSBO CellSBO(width*height, sizeof(struct cell));
	//setCells();

	AgentSBO.Bind(1);

	Shader vfShader = Shader("res/shaders/default.vert", "res/shaders/default.frag");

	Compute texCompute = Compute("res/shaders/texture.comp");
	std::cout << "TEX LINKED" << std::endl;
	Compute agentCompute = Compute("res/shaders/agent.comp");

	std::string nan = "nan";
	float viewAngle = 80*M_PI/180;	
	//glm::vec4 rot = glm::vec4(cos(viewAngle),-sin(viewAngle),sin(viewAngle), cos(viewAngle));
	glm::mat2 rotL = glm::mat2(cos(-viewAngle),-sin(-viewAngle),
							sin(-viewAngle), cos(-viewAngle));		
	
	glm::mat2 rotR = glm::mat2(cos(viewAngle),-sin(viewAngle),
							  sin(viewAngle), cos(viewAngle));
				  
	float speed = 0.05;
	int absx;
	int senseRange = 5;


	// texCompute.Activate();
	// glUniform1i(glGetUniformLocation(texCompute.ID, "PARTICLES"), NUM_PARTICLES);
	// texCompute.Dispatch(ceil(width / 8), ceil(height / 4), 1);
    while (!glfwWindowShouldClose(window))
    {
		timer();
		//int absx = int(floor((points[0].position.x+1)/2*width));
		// std::cout << points[0].position.x << "  " << points[0].position.y << "  " << points[0].position.z << std::endl;

		// std::cout << points[0].countest.x << "  " << points[0].countest.y << "  " << points[0].countest.z <<  " " << points[0].countest.w << std::endl;
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		agentCompute.Activate();
		AgentSBO.Bind(1);
		glUniform1i(glGetUniformLocation(agentCompute.ID, "PARTICLES"), NUM_PARTICLES);
		glUniform1f(glGetUniformLocation(agentCompute.ID, "dt"), timeDiff);
		glUniform1f(glGetUniformLocation(agentCompute.ID, "speed"), speed);
		glUniform1f(glGetUniformLocation(agentCompute.ID, "viewAngle"), viewAngle);
		glUniformMatrix2fv(glGetUniformLocation(agentCompute.ID, "rotL"), 1, GL_FALSE, glm::value_ptr(rotL));
		glUniformMatrix2fv(glGetUniformLocation(agentCompute.ID, "rotR"), 1, GL_FALSE, glm::value_ptr(rotR));
		glUniform1i(glGetUniformLocation(agentCompute.ID, "senseRange"), senseRange);

		agentCompute.Dispatch(ceil(NUM_PARTICLES / WORK_GROUP_SIZE), 1, 1);
		// glBindBuffer(GL_ARRAY_BUFFER, AgentSBO.ID);
		// glVertexPointer(4, GL_FLOAT, sizeof(struct agent), (void *)0);
		// glEnableClientState(GL_VERTEX_ARRAY);
		// glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
		// glDisableClientState(GL_VERTEX_ARRAY);
		// glBindBuffer(GL_ARRAY_BUFFER, 0);
		texCompute.Activate();
		glUniform1i(glGetUniformLocation(texCompute.ID, "PARTICLES"), NUM_PARTICLES);
		texCompute.Dispatch(ceil(width / 8), ceil(height / 4), 1);

		vfShader.Activate();

		Tex0.Bind(0);
		glUniform1i(glGetUniformLocation(vfShader.ID, "screen"), 0);
		VAO1.Bind();
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
		
    }   

	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	Tex0.Delete();

	AgentSBO.Delete();
	vfShader.Delete();
	texCompute.Delete();
	agentCompute.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

}

struct agent* setAgents()
{
	std::cout << NUM_PARTICLES << std::endl;
    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    struct agent *points = (struct agent *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct agent), bufMask);
    float dist, dir, z;
    for (int i = 0; i<NUM_PARTICLES;i++)
    {
		
        dist = ((float)(rand() % 1000)/1000)*0.85;
        dir = ((float)(rand() % 720)/360-1)*M_PI;
		z = ((float)(rand() % 720)/360 - 1);
		points[i].position.x = dist*cos(dir);
		points[i].position.y = dist*sin(dir);
        points[i].position.z = (dir+M_PI)/M_PI;//((float)(rand() % 720)/360 - 1);

        points[i].position.w = 1;
    }
	
	std::cout << "X0 COORD: " << points[0].position.x << std::endl;

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	return points;
};

GLFWwindow* setup()
{
	// Initialize GLFW
	glfwInit();
	std::cout << "STRUCT SIZE: " << sizeof(struct agent)<< std::endl;
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	
	GLFWwindow* window = glfwCreateWindow(width, height, "Compute Shader Test", NULL, NULL);
	// if (window == NULL)
	// {
	// 	std::cout << "Failed to create GLFW window" << std::endl;
	// 	glfwTerminate();
	// 	return NULL;
	// }
	glfwMakeContextCurrent(window);
	gladLoadGL();

	glViewport(0, 0, width, height);
	return window;
};

void timer()
{
	frames++;
	crntTime = glfwGetTime();
	timeDiff = crntTime - prevTime;
	if (WRITELOG)
	{
		if (coutDiff >= 10.0)
		{
			coutTime = crntTime;
			std::cout << frames/10 << std::endl;
			frames = 0;
		}
		coutDiff = crntTime - coutTime;
	}
	prevTime = crntTime;
};
// void setCells()
// {
//     GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
//     struct cell *points = (struct cell *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, width*height*sizeof(struct cell), bufMask);
//     for (int i = 0; i<width;i++)
//     {
// 		points[i].val = 0;
//     }
	
// 	std::cout << "X0 COORD: " << points[0].position.x << std::endl;

//     glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
// };


		// glBindBuffer(GL_ARRAY_BUFFER, AgentSBO.ID);
		// glVertexPointer(4, GL_FLOAT, sizeof(struct agent), (void *)0);
		// glEnableClientState(GL_VERTEX_ARRAY);
		// glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
		// glDisableClientState(GL_VERTEX_ARRAY);
		// glBindBuffer(GL_ARRAY_BUFFER, 0);