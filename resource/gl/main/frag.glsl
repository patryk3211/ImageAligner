#version 150 core

in vec2 p_UV;
flat in vec2 p_BorderWidth;

out vec4 o_FragColor;

uniform sampler2D u_Texture;
uniform vec2 u_Levels;

uniform int u_Flags;

#define FLAG_DRAW_BORDER 1
#define FLAG_DRAW_UNSELECTED 2

vec3 applyLevels(vec3 col, float m, float M) {
  return (clamp(col, m, M) - m) / (M - m);
}

void main() {
  vec4 texCol = texture(u_Texture, p_UV);
  vec3 color = applyLevels(texCol.xxx, u_Levels.x, u_Levels.y);

  if((u_Flags & FLAG_DRAW_UNSELECTED) != 0) {
    color *= vec3(1.0, 0.0, 0.0);
  }

  if((u_Flags & FLAG_DRAW_BORDER) != 0) {
    vec2 pos = p_UV * 2.0 - 1.0;
    if(abs(pos.x) > p_BorderWidth.x || abs(pos.y) > p_BorderWidth.y) {
      color = vec3(1.0, 0.0, 0.0);
    }
  }

  o_FragColor = vec4(color, 1.0);
}

