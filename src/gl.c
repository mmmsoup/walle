#include "gl.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

int gl_load_texture(gl_data_t *gl_data, int index, char *image_path) {
	if (index != 0 && index != 1) {
		ERR("gl_load_texture(): invalid index");
		return EXIT_FAILURE;
	}

	int image_width, image_height, num_channels;
	stbi_set_flip_vertically_on_load(1);
	unsigned char *image_data = stbi_load(image_path, &image_width, &image_height, &num_channels, 0);

	glGenTextures(1, &(gl_data->textures[index]));
	glBindTexture(GL_TEXTURE_2D, gl_data->textures[index]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	//glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(image_data);

	glUseProgram(gl_data->shader_program);
	char uniform_name[] = { 't', 'e', 'x', 't', 'u', 'r', 'e', index ? '1' : '0', '\0' };
	glUniform1i(glGetUniformLocation(gl_data->shader_program, uniform_name), index);

	return EXIT_SUCCESS;
}

int gl_init(gl_data_t *gl_data, Display *display, XVisualInfo *visual_info, Window window) {
	gl_data->display = display;
	gl_data->window = window;
	gl_data->mixing = 1.0f;

	gl3wInit();

	GLXContext glx_context = glXCreateContext(display, visual_info, NULL, GL_TRUE);
	glXMakeCurrent(gl_data->display, gl_data->window, glx_context);

	glEnable(GL_DEPTH_TEST);

	const GLchar *vertex_shader_source =
		"#version 330 core\n"
		"layout (location = 0) in vec3 pos;\n"
		"layout (location = 1) in vec2 intexcoord;\n"
		"out vec2 texcoord;\n"
		"void main() { \n"
		"gl_Position = vec4(pos, 1.0);\n"
		"texcoord = intexcoord;\n"
		"}";
	const GLchar *fragment_shader_source =
		"#version 330 core\n"
		"in vec2 texcoord;\n"
		"uniform sampler2D texture0;\n"
		"uniform sampler2D texture1;\n"
		"uniform float mixing;\n"
		"out vec4 FragColor;\n"
		"void main() { \n"
		"FragColor = mix(texture2D(texture0, texcoord), texture2D(texture1, texcoord), mixing);\n"
		"}";
	GLint success = GL_TRUE;
	GLsizei log_length;

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
		GLchar log_buffer[log_length - 1];
		glGetShaderInfoLog(vertex_shader, log_length - 1, NULL, log_buffer);
		ERR("glCompileShader(): %s", log_buffer);
	}
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
		GLchar log_buffer[log_length - 1];
		glGetShaderInfoLog(fragment_shader, log_length - 1, NULL, log_buffer);
		ERR("glCompileShader(): %s", log_buffer);
	}
	gl_data->shader_program = glCreateProgram();
	glAttachShader(gl_data->shader_program, vertex_shader);
    glAttachShader(gl_data->shader_program, fragment_shader);
    glLinkProgram(gl_data->shader_program);
    glGetShaderiv(gl_data->shader_program, GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		glGetProgramiv(gl_data->shader_program, GL_INFO_LOG_LENGTH, &log_length);
		GLchar log_buffer[log_length - 1];
		glGetProgramInfoLog(gl_data->shader_program, log_length - 1, NULL, log_buffer);
		ERR("glLinkProgram(): %s", log_buffer);
	}
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

	GLfloat vertices[] = {
		1.0f,	1.0f,	0.0f,	1.0f,	1.0f,
		1.0f,	-1.0f,	0.0f,	1.0f,	0.0f,
		-1.0f,	-1.0f,	0.0f,	0.0f,	0.0f,
		-1.0f,	1.0f,	0.0f,	0.0f,	1.0f,
	};
	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};
	GLuint vertex_buffer_object, element_buffer_object;
	glGenVertexArrays(1, &(gl_data->vertex_array_object));
	glGenBuffers(1, &vertex_buffer_object);
	glGenBuffers(1, &element_buffer_object);
	glBindVertexArray(gl_data->vertex_array_object);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	return EXIT_SUCCESS;
}

int gl_resize(gl_data_t *gl_data) {
	Window root;
	int x, y;
	unsigned int width, height, border_width, depth;
	XGetGeometry(gl_data->display, gl_data->window, &root, &x, &y, &width, &height, &border_width, &depth);
	glViewport(0, 0, width, height);
	glFlush();
	return EXIT_SUCCESS;
}

int gl_render(gl_data_t *gl_data) {
	gl_data->mixing = 1.0f - gl_data->mixing;
	glUniform1f(glGetUniformLocation(gl_data->shader_program, "mixing"), gl_data->mixing);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl_data->textures[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gl_data->textures[1]);

	glUseProgram(gl_data->shader_program);

	glBindVertexArray(gl_data->vertex_array_object);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glFlush();
	glXSwapBuffers(gl_data->display, gl_data->window);

	return EXIT_SUCCESS;
}
