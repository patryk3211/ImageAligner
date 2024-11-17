#version 150 core

in vec2 p_Pos;
flat in vec2 p_BorderWidth;

out vec4 o_FragColor;

uniform float u_Scale;
uniform vec4 u_Selection;
uniform float u_Width;

#define DASH_SCALE 0.075

void main() {
  vec2 centerPos = p_Pos * 2.0 - 1.0;

  // Make the border dashed
  vec2 scaledPos = p_Pos * u_Selection.zw * u_Width * u_Scale;
  bool solid = int((scaledPos.x + scaledPos.y) * DASH_SCALE) % 2 == 0;

  if((abs(centerPos.x) > p_BorderWidth.x || abs(centerPos.y) > p_BorderWidth.y) && solid) {
    o_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  } else {
    discard;
  }
}

