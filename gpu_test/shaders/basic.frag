#version 330

uniform sampler2D blur_y;
uniform vec2 resolution;
in vec2 texture_coord;
out vec4 color_out;

void main(void) {
  color_out = texture(blur_y, texture_coord);
}
