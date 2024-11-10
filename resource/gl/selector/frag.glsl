#version 150 core

in vec2 p_Pos;
in vec2 p_BorderWidth;

out vec4 o_FragColor;

uniform vec4 u_Selection;
uniform float u_Scale;

void main() {
  float centerDist = max(abs(p_Pos.x), abs(p_Pos.y));
  if(abs(p_Pos.x) > p_BorderWidth.x || abs(p_Pos.y) > p_BorderWidth.y) {
    o_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  } else {
    discard;
  }
}

