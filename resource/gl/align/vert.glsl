#version 150 core

const vec2 c_Position[4] = vec2[4](
  vec2(-1.0, -1.0),
  vec2(-1.0,  1.0),
  vec2( 1.0, -1.0),
  vec2( 1.0,  1.0)
);

const vec2 c_UV[4] = vec2[4](
  vec2(0.0, 1.0),
  vec2(0.0, 0.0),
  vec2(1.0, 1.0),
  vec2(1.0, 0.0)
);

out vec2 p_UV;

void main() {
  gl_Position = vec4(c_Position[gl_VertexID], 0.0, 1.0);
  p_UV = c_UV[gl_VertexID];
}

