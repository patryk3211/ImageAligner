#version 150 core

in vec2 a_Position;
in vec2 a_UV;

out vec2 p_UV;
flat out vec2 p_BorderWidth;

uniform mat4 u_View;
uniform mat3 u_Transform;

const float BORDER_WIDTH = 0.01;

void main() {
  vec3 imgPos = u_Transform * vec3(a_Position, 1.0);
  imgPos.xy /= imgPos.z;

  gl_Position = u_View * vec4(imgPos.xy, 0.0, 1.0);

  p_UV = a_UV;

  vec2 scale = abs(u_View * vec4(1.0, 1.0, 0.0, 0.0)).xy;
  p_BorderWidth = 1.0 - BORDER_WIDTH / scale;
}

