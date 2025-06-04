#include "gl.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// try to mimic feh's '--bg-fill' option (i.e no horrible stretched or repeated textures)
// TODO: I don't think this is quite right but I'll come back to it...
int gl_set_texture_multiplier(gl_data_t *gl_data, int index) {
	char uniform_name[] = { 't', 'e', 'x', index ? '1' : '0', 'm', 'u', 'l', 't', '\0' };
	double width_ratio = (double)gl_data->textures[0].width/gl_data->window_width;
	double height_ratio = (double)gl_data->textures[0].height/gl_data->window_height;
	glUniform2f(glGetUniformLocation(gl_data->shader_program, uniform_name), width_ratio > height_ratio ? height_ratio/width_ratio : 1.0f, width_ratio > height_ratio ? 1.0f : width_ratio/height_ratio);
	return EXIT_SUCCESS;
}

int gl_load_texture(gl_data_t *gl_data, int index, char *image_path) {
	if (index != 0 && index != 1) {
		ERR("gl_load_texture(): invalid index");
		return EXIT_FAILURE;
	}

	if (gl_data->textures[index].id != 0) glDeleteTextures(1, &(gl_data->textures[index].id));

	unsigned char *image_data;
	void (*fr)(void*) = free;

	if (image_path[0] == '/') {
		int num_channels;
		stbi_set_flip_vertically_on_load(1);
		image_data = stbi_load(image_path, &(gl_data->textures[index].width), &(gl_data->textures[index].height), &num_channels, 3);
		if (image_data == NULL) {
			ERR("stbi_load(): failure of some description?");
			return EXIT_FAILURE;
		}
		fr = stbi_image_free;
	} else {
		// silly way to set background to single colour using textures because I'm scared of changing the OpenGL code aaaaaaa
		image_data = malloc(3*sizeof(char));
		image_data[0] = hex_char_val(image_path[1]) * 16 + hex_char_val(image_path[2]);
		image_data[1] = hex_char_val(image_path[3]) * 16 + hex_char_val(image_path[4]);
		image_data[2] = hex_char_val(image_path[5]) * 16 + hex_char_val(image_path[6]);
		gl_data->textures[index].width = 1;
		gl_data->textures[index].height = 1;
	}

	glGenTextures(1, &(gl_data->textures[index].id));
	glBindTexture(GL_TEXTURE_2D, gl_data->textures[index].id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, gl_data->textures[index].width, gl_data->textures[index].height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	fr(image_data);

	glUseProgram(gl_data->shader_program);
	char uniform_name[] = { 't', 'e', 'x', index + 0x30, '\0' };
	glUniform1i(glGetUniformLocation(gl_data->shader_program, uniform_name), index);

	gl_set_texture_multiplier(gl_data, index);

	return EXIT_SUCCESS;
}

int gl_load_shaders(GLuint *program, char *custom_vertex_source, int vertex_source_len, char *custom_fragment_source, int fragment_source_len) {
	const char *vertex_source = custom_vertex_source == NULL ? default_vertex_shader_source : custom_vertex_source;
	const char *fragment_source = custom_fragment_source == NULL ? default_fragment_shader_source : custom_fragment_source;

	vertex_source_len = custom_vertex_source == NULL ? default_vertex_shader_source_len : vertex_source_len;
	fragment_source_len = custom_fragment_source == NULL ? default_fragment_shader_source_len : fragment_source_len;

	GLint success = GL_TRUE;
	GLsizei log_length;

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, &vertex_source_len);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
		GLchar log_buffer[log_length - 1];
		glGetShaderInfoLog(vertex_shader, log_length - 1, NULL, log_buffer);
		ERR("glCompileShader(): %s", log_buffer);
		return EXIT_FAILURE;
	}
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, &fragment_source_len);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
		GLchar log_buffer[log_length - 1];
		glGetShaderInfoLog(fragment_shader, log_length - 1, NULL, log_buffer);
		ERR("glCompileShader(): %s", log_buffer);
		return EXIT_FAILURE;
	}
	*program = glCreateProgram();
	glAttachShader(*program, vertex_shader);
    glAttachShader(*program, fragment_shader);
    glLinkProgram(*program);
    glGetShaderiv(*program, GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &log_length);
		GLchar log_buffer[log_length - 1];
		glGetProgramInfoLog(*program, log_length - 1, NULL, log_buffer);
		ERR("glLinkProgram(): %s", log_buffer);
		return EXIT_FAILURE;
	}
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

	return EXIT_SUCCESS;
}

int gl_init(gl_data_t *gl_data, Display *display, XVisualInfo *visual_info, Window window) {
	memset(gl_data, 0, sizeof(gl_data_t));
	gl_data->display = display;
	gl_data->window = window;
	gl_data->mixing = 1.0f;
	gl_data->texture_transition.active = 0;

	gl3wInit();

	GLXContext glx_context = glXCreateContext(display, visual_info, NULL, GL_TRUE);
	if (!glXMakeCurrent(gl_data->display, gl_data->window, glx_context)) {
		ERR("glXMakeCurrent()");
		return EXIT_FAILURE;
	}

	glEnable(GL_DEPTH_TEST);

	if (gl_load_shaders(&(gl_data->shader_program), NULL, 0, NULL, 0) != EXIT_SUCCESS) return EXIT_FAILURE;

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

	// avoid possible shearing from pictures with odd dimensions
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int i = 0; i < 2; i++) {
		glGenTextures(1, &(gl_data->textures[i].id));
		glBindTexture(GL_TEXTURE_2D, gl_data->textures[i].id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	gl_show_texture(gl_data, 0, 0);

	return EXIT_SUCCESS;
}

// duration: in milliseconds
int gl_show_texture(gl_data_t *gl_data, int index, double duration) {
	if (duration == 0) {
		gl_data->current_texture = index;
		glUniform1f(glGetUniformLocation(gl_data->shader_program, "mixing"), index);
	} else {
		DEBUG("starting continuous rendering");
		gl_data->texture_transition.duration = duration;
		gl_data->texture_transition.active = 1;
		gl_data->texture_transition.finish = (double)clock() / CLOCKS_PER_MSEC + gl_data->texture_transition.duration;
	}

	return EXIT_SUCCESS;
}

int gl_resize(gl_data_t *gl_data) {
	Window root;
	int x, y;
	unsigned int border_width, depth;
	XGetGeometry(gl_data->display, gl_data->window, &root, &x, &y, &(gl_data->window_width), &(gl_data->window_height), &border_width, &depth);
	glViewport(0, 0, gl_data->window_width, gl_data->window_height);

	gl_set_texture_multiplier(gl_data, 0);
	gl_set_texture_multiplier(gl_data, 1);

	glFlush();

	return EXIT_SUCCESS;
}

int gl_render(gl_data_t *gl_data) {
	double current_time = (double)clock() / CLOCKS_PER_MSEC;

	if (gl_data->texture_transition.active) {
		if (gl_data->texture_transition.finish <= current_time) {
			gl_data->texture_transition.active = 0;
			gl_data->current_texture = 1 - gl_data->current_texture;
			glUniform1f(glGetUniformLocation(gl_data->shader_program, "mixing"), gl_data->current_texture);
			DEBUG("stopping continuous rendering");
		} else {
			glUniform1f(glGetUniformLocation(gl_data->shader_program, "mixing"), (1 - gl_data->current_texture) + (gl_data->current_texture ? 1 : -1) * (gl_data->texture_transition.finish-current_time)/gl_data->texture_transition.duration);
		}
	}

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl_data->textures[0].id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gl_data->textures[1].id);

	glUseProgram(gl_data->shader_program);

	glBindVertexArray(gl_data->vertex_array_object);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glFlush();
	glXSwapBuffers(gl_data->display, gl_data->window);

	return EXIT_SUCCESS;
}

int stbi_valid(char *filename) {
	FILE *file = stbi__fopen(filename, "rb");
	if (!file) return 0;

	stbi__context s;
	stbi__start_file(&s, file);

	int valid = (0 ||
#ifndef STBI_NO_PNG
		stbi__png_test(&s) ||
#endif
#ifndef STBI_NO_BMP
		stbi__bmp_test(&s) ||
#endif
#ifndef STBI_NO_GIF
		stbi__gif_test(&s) ||
#endif
#ifndef STBI_NO_PSD
		stbi__psd_test(&s) ||
#endif
#ifndef STBI_NO_PIC
		stbi__pic_test(&s) ||
#endif
#ifndef STBI_NO_JPEG
		stbi__jpeg_test(&s) ||
#endif
#ifndef STBI_NO_PNM
		stbi__pnm_test(&s) ||
#endif
#ifndef STBI_NO_HDR
		stbi__hdr_test(&s) ||
#endif
#ifndef STBI_NO_TGA
		stbi__tga_test(&s) ||
#endif
		0);

	fclose(file);

	return valid;
}
