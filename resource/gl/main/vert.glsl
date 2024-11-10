#version 150 core

in vec2 a_Position;
in vec2 a_UV;

out vec2 p_UV;

// uniform mat4 u_Proj;
uniform mat4 u_View;
uniform mat3 u_Transform;

void main() {
  vec3 imgPos = u_Transform * vec3(a_Position, 1.0);
  gl_Position = u_View * vec4(imgPos.xy, 0.0, 1.0);

  p_UV = a_UV;
}

