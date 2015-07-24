#version 330
uniform sampler2D diffuse_texture;
uniform vec2 resolution;
in vec2 texture_coord;
out vec4 color_out;

int filter_size = 13;
int filter_mid = 6;
float kernel[13] = float[](0.0481169041, 0.0599573546, 0.0717819860, 0.0825689377, 0.0912527887, 0.0968955460, 0.0988529659, 0.0968955460, 0.0912527887, 0.0825689377, 0.0717819860, 0.0599573546, 0.0481169041);

// Blurs in X direction
void main(void) {
  vec4 sum = vec4(0);
  int j;
  int i;

  for (i = 0; i < filter_size; i++) {
    vec4 test = texture2D(diffuse_texture, texture_coord + vec2(filter_mid - i, 0)/resolution);
    sum += test * kernel[i];
  }

  color_out = sum;
}
