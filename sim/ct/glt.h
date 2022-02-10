#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef gltError
#define gltError _gltPanic
#endif

#ifndef GLT_IMPL

typedef struct GLTvertexStore
{
	GLuint vao;

	GLuint* vbos;
	int num_buffers;
} GLTvertexStore;

void gltDeleteVertexStore(GLTvertexStore vs);
GLTvertexStore gltCreateVertexStore(int num_buffers);
void gltUseVertexStoreBuffer(GLTvertexStore vs, int id);
void gltVertexStoreSetData(GLTvertexStore vs, int id, unsigned int size, void* data, int draw_type);
GLuint gltCreateShader(const char* vert_filename, char* frag_filename);
GLFWwindow* gltCreateDefaultContext(int width, int height, const char* name, void (*setupFunc)(void));
void gltUseVertexStore(GLTvertexStore vs);

#else

typedef struct GLTvertexStore
{
	GLuint vao;

	GLuint* vbos;
	int num_buffers;
} GLTvertexStore;

void _gltPanic(const char* err)
{
	fprintf(stderr, "%s\n", err);
	exit(1);
}

void gltDeleteVertexStore(GLTvertexStore vs)
{
	glDeleteVertexArrays(1, &vs.vao);
	glDeleteBuffers(vs.num_buffers, vs.vbos);
	free(vs.vbos);
}

GLTvertexStore gltCreateVertexStore(int num_buffers)
{
	GLTvertexStore ret;

	ret.num_buffers = num_buffers;
	ret.vbos = (GLuint*) malloc(sizeof(GLuint) * num_buffers);

	glGenVertexArrays(1, &ret.vao);
	glGenBuffers(num_buffers, ret.vbos);

	return ret;
}

void gltUseVertexStoreBuffer(GLTvertexStore vs, int id)
{
	if (id >= vs.num_buffers)
		gltError("Invalid ID");

	glBindBuffer(GL_ARRAY_BUFFER, vs.vbos[id]);
	glBindVertexArray(vs.vao);
}

void gltUseVertexStore(GLTvertexStore vs)
{
	glBindVertexArray(vs.vao);
}

void gltVertexStoreSetData(GLTvertexStore vs, int id, unsigned int size, void* data, int draw_type)
{
	gltUseVertexStoreBuffer(vs, id);
	glBufferData(GL_ARRAY_BUFFER, size, data, draw_type);
}

char* _gltReadFile(FILE* fp)
{
	int i = 0;
	char* ret = (char*) malloc(1);
	char c;

	while ((c = getc(fp)) != EOF)
	{
		ret[i++] = c;
		ret = (char*) realloc(ret, i + 1);
	}

	ret[i] = '\0';
	return ret;
}

GLuint _gltCompileShader(int shader_type, const char* const* shader_src)
{
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, shader_src, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		GLint max_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

		char buffer[max_len];
		glGetShaderInfoLog(shader, max_len, NULL, buffer);
		glDeleteShader(shader);

		fprintf(stderr, "SHADER COMPILATION FAILED: %s\n", buffer);
	}

	return shader;
}

GLuint _gltBasicShaderProg(GLuint vert, GLuint frag)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	GLint success;
	glGetShaderiv(program, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE)
	{
		GLint max_len = 0;
		glGetShaderiv(program, GL_INFO_LOG_LENGTH, &max_len);

		char buffer[max_len];
		glGetShaderInfoLog(program, max_len, NULL, buffer);
		glDeleteShader(program);

		fprintf(stderr, "SHADER LINKING FAILED: %s\n", buffer);
	}

	return program;
}

GLuint gltCreateShader(const char* vert_filename, const char* frag_filename)
{
	FILE* vert_file = fopen(vert_filename, "r");
	if (vert_file == NULL)
	{
		fprintf(stderr, "Failed to open %s\n", vert_filename);
		exit(1);
	}

	FILE* frag_file = fopen(frag_filename, "r");
	if (frag_file == NULL)
	{
		fprintf(stderr, "Failed to open %s\n", vert_filename);
		exit(1);
	}

	const char* vert_source = _gltReadFile(vert_file);
	const char* frag_source = _gltReadFile(frag_file);

	GLuint vertex_shader = _gltCompileShader(GL_VERTEX_SHADER, &vert_source);
	GLuint fragment_shader = _gltCompileShader(GL_FRAGMENT_SHADER, &frag_source);
	GLuint id = _gltBasicShaderProg(vertex_shader, fragment_shader);

	fclose(vert_file);
	fclose(frag_file);

	free((void*) vert_source);
	free((void*) frag_source);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return id;
}

GLFWwindow* gltCreateDefaultContext(int width, int height, const char* name, void (*setupFunc)(void))
{
	if (glfwInit() != GLFW_TRUE)
		gltError("Failed to init gltw");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	if (setupFunc)
		setupFunc();

	GLFWwindow* window = glfwCreateWindow(width, height, name, NULL, NULL);
	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
		gltError("Failed to init glew");

	return window;
}

#endif
