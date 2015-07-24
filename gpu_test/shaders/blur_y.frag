#version 330
uniform sampler2D blur_x;
uniform vec2 resolution;
in vec2 texture_coord;
out vec4 color_out;

int filter_size = 13;
int filter_mid = 7;
float kernel[13] = float[](0.0481169041, 0.0599573546, 0.0717819860, 0.0825689377, 0.0912527887, 0.0968955460, 0.0988529659, 0.0968955460, 0.0912527887, 0.0825689377, 0.0717819860, 0.0599573546, 0.0481169041);

// Blurs in Y direction
void main(void) {
  vec4 sum = vec4(0);
  int j;
  int i;

  for (i = 0; i < filter_size; i++) {
    sum += texture2D(blur_x, texture_coord + vec2(0, filter_mid - i)/resolution) * kernel[i];
  }

  color_out = sum;
}
