#version 330
uniform sampler2D diffuse_texture;
uniform sampler2D blur_y;
uniform vec2 resolution;
in vec2 texture_coord;
out vec4 color_out;

float amount = 20.0;


// Generates glow
void main(void) {
  vec4 base_color = texture2D(diffuse_texture, texture_coord);
  if (base_color.a < 1.0) {
    vec4 sum = texture2D(blur_y, texture_coord);
    color_out = mix(base_color, sum, 0.9);
  } else {
    color_out = base_color;
  }
}
