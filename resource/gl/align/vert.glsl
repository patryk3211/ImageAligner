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

out vec2 p_RefUV;
out vec2 p_AlignUV;

uniform vec4 u_ViewSelection;
uniform mat3 u_Homography;

void main() {
  gl_Position = vec4(c_Position[gl_VertexID], 0.0, 1.0);

  p_RefUV = u_ViewSelection.xy + c_UV[gl_VertexID] * u_ViewSelection.zw;

  vec3 projectedUV = u_Homography * vec3(p_RefUV, 1.0);
  projectedUV /= projectedUV.z;
  p_AlignUV = projectedUV.xy;
}

