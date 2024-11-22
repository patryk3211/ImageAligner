#version 150 core

in vec2 p_UV;
in vec3 p_Color;

out vec4 o_FragColor;

#define THICKNESS 0.05

void main() {
  vec4 color = vec4(p_Color, 1.0);

  float centDist = length(p_UV);
  if(centDist <= 0.5) {
    if(centDist >= 0.5 - THICKNESS) {
      o_FragColor = color;
      return;
    }
    if(p_UV.y <= 0 && abs(p_UV.x) < (THICKNESS / 2.0)) {
      o_FragColor = color;
      return;
    }
  }

  discard;
}

