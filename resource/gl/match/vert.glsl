#version 150 core

in vec2 a_Position;
in vec3 a_Color;

out vec3 p_Color;

uniform mat4 u_View;

void main() {
  gl_Position = u_View * vec4(a_Position, 0.0, 1.0);
  p_Color = a_Color;
}

