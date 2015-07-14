#version 330
uniform sampler2D diffuse_texture;
uniform vec2 resolution;
in vec2 texture_coord;
out vec4 color_out;

int sample_range = 10;
float amount = 1.0;
// Glow effect
void main(void) {

  color_out = texture2D(diffuse_texture, texture_coord);
  vec4 sum = vec4(0);
  int j;
  int i;

  for( i= -sample_range + 1 ;i < sample_range; i++) {
    for (j = -sample_range + 1; j < sample_range; j++) {
      sum += amount*texture2D(diffuse_texture, texture_coord + vec2(j, i)/resolution) / (3.f + j*j + i*i);
    }
  }
  if (color_out.r < 0.3) {
    color_out = mix(color_out, sum*sum, 0.12);
  } else {
    if (color_out.r < 0.5) {
      color_out = mix(color_out, sum*sum, 0.09);
    } else {
      color_out = mix(color_out, sum*sum, 0.075);
    }
  }
}
