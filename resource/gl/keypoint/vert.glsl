#version 150 core

in vec2 a_Position;
in vec2 a_UV;

out vec2 p_UV;

uniform mat4 u_View;

void main() {
  gl_Position = u_View * vec4(a_Position, 0.0, 1.0);
  p_UV = a_UV;
}

