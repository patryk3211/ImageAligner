#version 150 core

in vec2 p_UV;

out vec4 o_FragColor;

uniform sampler2D u_Texture;
uniform float u_ColorMult;
uniform int u_Highlight;

void main() {
  vec4 texCol = texture(u_Texture, p_UV) * u_ColorMult;

  vec2 centerCoords = p_UV * 2.0 - 1.0;
  float centDist = max(abs(centerCoords.x), abs(centerCoords.y));
  if(u_Highlight > 0 && centDist > 0.95) {
    o_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  } else {
    o_FragColor = vec4(texCol.xxx, 1.0);
  }
}

