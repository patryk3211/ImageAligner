#version 150 core

const vec2 c_Position[4] = vec2[4](
  vec2(0.0, 0.0),
  vec2(0.0, 1.0),
  vec2(1.0, 0.0),
  vec2(1.0, 1.0)
);

out vec2 p_Pos;
flat out vec2 p_BorderWidth;

uniform vec4 u_Selection;
uniform mat4 u_View;
uniform float u_Scale;

const float BORDER_WIDTH = 0.01;

void main() {
  gl_Position = u_View * vec4((u_Selection.xy + c_Position[gl_VertexID] * u_Selection.zw), 0.0, 1.0);

  p_Pos = (c_Position[gl_VertexID] * 2.0 - 1.0);
  p_BorderWidth = 1.0 - BORDER_WIDTH / abs(u_Selection.zw) / u_Scale;
}

