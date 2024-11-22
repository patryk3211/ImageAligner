#version 150 core

in vec3 p_Color;

out vec4 o_FragColor;

void main() {
  o_FragColor = vec4(p_Color, 1.0);
}

