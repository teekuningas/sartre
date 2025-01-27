#include <vector>
#include <ostream>
#include <iostream>

#include "shader_utils.h"

GLuint loadShader(GLenum type, const std::string& source)
{
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		std::vector<GLchar> infoLog(infoLen);
		glGetShaderInfoLog(shader, infoLen, &infoLen, &infoLog[0]);
		std::cerr << "Error compiling shader:\n" << &infoLog[0] << std::endl;
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint createProgram(const std::string& vertexSource, const std::string& fragmentSource)
{
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		std::vector<GLchar> infoLog(infoLen);
		glGetProgramInfoLog(program, infoLen, &infoLen, &infoLog[0]);
		std::cerr << "Error linking program:\n" << &infoLog[0] << std::endl;
		glDeleteProgram(program);
		return 0;
	}

	return program;
}
