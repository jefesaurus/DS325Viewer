#version 330

uniform sampler2D diffuse_texture;
uniform vec2 resolution;
in vec2 texture_coord;
out vec4 color_out;

void main(void) {
  color_out = texture(diffuse_texture, texture_coord);
}
