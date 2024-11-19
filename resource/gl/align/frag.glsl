#version 150 core

in vec2 p_RefUV;
in vec2 p_AlignUV;

out vec4 o_FragColor;

uniform sampler2D u_RefTexture;
uniform sampler2D u_AlignTexture;

uniform int u_DisplayType;
uniform float u_DisplayParam;

uniform vec2 u_RefLevels;
uniform vec2 u_AlignLevels;

vec3 applyLevels(vec3 input, float m, float M) {
  return (clamp(input, m, M) - m) / (M - m);
}

void main() {
  vec3 colRef;
  // TODO: Use border clamping for this
  if(p_RefUV.x >= 0.0 && p_RefUV.x <= 1.0 && p_RefUV.y >= 0.0 && p_RefUV.y <= 1.0) {
    colRef = texture(u_RefTexture, p_RefUV).xyz;
  } else {
    colRef = vec3(0.0, 0.0, 0.0);
  }
  vec3 colAli;// = texture(u_AlignTexture, p_UV) * 10.0;
  if(p_AlignUV.x >= 0.0 && p_AlignUV.x <= 1.0 && p_AlignUV.y >= 0.0 && p_AlignUV.y <= 1.0) {
    colAli = texture(u_AlignTexture, p_AlignUV).xyz;
  } else {
    colAli = vec3(0.0, 0.0, 0.0);
  }

  colRef = applyLevels(colRef, u_RefLevels.x, u_RefLevels.y);
  colAli = applyLevels(colAli, u_AlignLevels.x, u_AlignLevels.y);

  vec3 color;// = colRef.xxx;
  float diff;
  switch(u_DisplayType) {
    case 0:
      color = vec3(mix(colRef.x, colAli.x, u_DisplayParam));
      break;
    case 1:
      color = vec3((colRef.x - colAli.x) * u_DisplayParam / 2.0 + 0.5);
      break;
    case 2:
      diff = (colRef.x - colAli.x) * u_DisplayParam;
      color = vec3(max(diff, 0), -min(diff, 0), abs(diff));
      // color = vec3(colRef.x, colAli.x, abs(colRef.x - colAli.x) * u_DisplayParam);
      break;
  }
  o_FragColor = vec4(color, 1.0);
}

