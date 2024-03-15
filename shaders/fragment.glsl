#version 330 core

in vec2 texcoord;

uniform sampler2D tex0;
uniform vec2 tex0mult;
uniform sampler2D tex1;
uniform vec2 tex1mult;
uniform float mixing;

out vec4 FragColor;

void main() { 
	FragColor = mix(texture2D(tex0, texcoord * tex0mult), texture2D(tex1, texcoord * tex1mult), mixing);
}
