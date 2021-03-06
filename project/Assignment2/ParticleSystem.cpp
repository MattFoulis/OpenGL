#include "ParticleSystem.h"

#define GLM_ENABLE_EXPERIMENTAL

#include <vector>
#include <algorithm>
#include <glm/gtx/norm.hpp>
#include "soil.h"

using namespace std;

ParticleSystem::ParticleSystem()
{
	
	maxParticles = 10000;
	LastUsedParticle = 0;

	//ParticlesContainer.resize(maxParticles);
}

ParticleSystem::~ParticleSystem()
{

}

int ParticleSystem::findUnusedParticle()
{
	for (int i = LastUsedParticle; i < maxParticles; i++)
	{
		if (ParticlesContainer[i].life < 0)
		{
			LastUsedParticle = i;
			return i;
		}
	}

	for (int i = 0; i < LastUsedParticle; i++)
	{
		if (ParticlesContainer[i].life < 0)
		{
			LastUsedParticle = i;
			return i;
		}
	}

	return 0; //All particles are taken, override the first one
}

void ParticleSystem::sortParticles()
{
	sort(&ParticlesContainer[0], &ParticlesContainer[maxParticles]);
}

void ParticleSystem::makeParticleSystem(GLuint program)
{
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	programID = program;

	g_particle_position_size_data = new GLfloat[maxParticles * 4];
	g_particle_color_data = new GLubyte[maxParticles * 4];

	for (int i = 0; i < maxParticles; i++)
	{
		ParticlesContainer[i].life = -1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}

	static const GLfloat g_vertex_buffer_data[] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
		 0.5f,  0.5f, 0.0f,
	};

	
	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	// The VBO containing the positions and sizes of the particles
	//particles_position_buffer;
	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	// The VBO containing the colors of the particles
	//particles_color_buffer;
	glGenBuffers(1, &particles_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

	lastTime = glfwGetTime();

	CameraRight_worldspace_ID = glGetUniformLocation(programID, "CameraRight_worldspace");
	CameraUp_worldspace_ID = glGetUniformLocation(programID, "CameraUp_worldspace");


	Texture = SOIL_load_OGL_texture("..\\..\\images\\gravel.jpg",
		SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT);

	if (Texture == 0)
	{
		printf("TexID soil loading error: '%s'", SOIL_last_result());
	}
}

void ParticleSystem::drawParticleSystem(glm::mat4 ProjectionMatrix, glm::mat4 ViewMatrix)
{

	glm::vec3 CameraPosition(glm::inverse(ViewMatrix)[3]);
	glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	
	double currentTime = glfwGetTime();
	double delta = currentTime - lastTime;
	lastTime = currentTime;

	glBindVertexArray(VertexArrayID);

	int newparticles = (int)(delta * 10000.0);
	if (newparticles > (int)(0.016f * 10000.0))
		newparticles = (int)(0.016f * 10000.0);

	for (int i = 0; i < newparticles; i++) {
		int particleIndex = findUnusedParticle();
		ParticlesContainer[particleIndex].life = 5.0f; // This particle will live 5 seconds.
		ParticlesContainer[particleIndex].pos = glm::vec3(0, 0, -20.0f);

		float spread = 1.5f;
		glm::vec3 maindir = glm::vec3(0.0f, 10.0f, 0.0f);
		// Very bad way to generate a random direction; 
		// See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
		// combined with some user-controlled parameters (main direction, spread, etc)
		glm::vec3 randomdir = glm::vec3(
			(rand() % 2000 - 1000.0f) / 1000.0f,
			(rand() % 2000 - 1000.0f) / 1000.0f,
			(rand() % 2000 - 1000.0f) / 1000.0f
		);

		ParticlesContainer[particleIndex].speed = maindir + randomdir * spread;


		// Very bad way to generate a random color
		ParticlesContainer[particleIndex].r = rand() % 256;
		ParticlesContainer[particleIndex].g = rand() % 256;
		ParticlesContainer[particleIndex].b = rand() % 256;
		ParticlesContainer[particleIndex].a = (rand() % 256) / 3;

		ParticlesContainer[particleIndex].size = (rand() % 1000) / 2000.0f + 0.1f;

	}

	int ParticlesCount = 0;
	for (int i = 0; i < maxParticles; i++) {

		Particle& p = ParticlesContainer[i]; // shortcut

		if (p.life > 0.0f) {

			// Decrease life
			p.life -= delta;
			if (p.life > 0.0f) {

				// Simulate simple physics : gravity only, no collisions
				p.speed += glm::vec3(0.0f, -9.81f, 0.0f) * (float)delta * 0.5f;
				p.pos += p.speed * (float)delta;
				p.cameradistance = glm::length(p.pos - CameraPosition);
				//ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

				// Fill the GPU buffer
				g_particle_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
				g_particle_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
				g_particle_position_size_data[4 * ParticlesCount + 2] = p.pos.z;

				g_particle_position_size_data[4 * ParticlesCount + 3] = p.size;

				g_particle_color_data[4 * ParticlesCount + 0] = p.r;
				g_particle_color_data[4 * ParticlesCount + 1] = p.g;
				g_particle_color_data[4 * ParticlesCount + 2] = p.b;
				g_particle_color_data[4 * ParticlesCount + 3] = p.a;

			}
			else {
				// Particles that just died will be put at the end of the buffer in SortParticles();
				p.cameradistance = -1.0f;
			}

			ParticlesCount++;

		}
	}

	sortParticles();


	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particle_position_size_data);

	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particle_color_data);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(programID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glUniform1i(TextureID, 0);

	glUniform3f(CameraRight_worldspace_ID, ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
	glUniform3f(CameraUp_worldspace_ID, ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);

	glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &ViewProjectionMatrix[0][0]);


	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : positions of particles' centers
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size : x + y + z + size => 4
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// 3rd attribute buffer : particles' colors
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	glVertexAttribPointer(
		2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size : r + g + b + a => 4
		GL_UNSIGNED_BYTE,                 // type
		GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// These functions are specific to glDrawArrays*Instanced*.
	// The first parameter is the attribute buffer we're talking about.
	// The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
	// http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
	glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
	glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
	glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1


	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

	glDisable(GL_BLEND);
}

void ParticleSystem::defineUnions()
{
	glBindVertexArray(VertexArrayID);
	glUseProgram(programID);

	CameraRight_worldspace_ID = glGetUniformLocation(programID, "CameraRight_worldspace");
	CameraUp_worldspace_ID = glGetUniformLocation(programID, "CameraUp_worldspace");
	ViewProjMatrixID = glGetUniformLocation(programID, "VP");

	TextureID = glGetUniformLocation(programID, "myTextureSampler");
}