#version 150 core

in vec2 p_UV;

out vec4 o_FragColor;

uniform sampler2D u_RefTexture;
uniform sampler2D u_AlignTexture;

uniform int u_DisplayType;
uniform float u_DisplayParam;

uniform vec4 u_ViewSelection;
uniform mat3 u_Homography;

void main() {
  vec2 refUV = u_ViewSelection.xy + p_UV * u_ViewSelection.zw;
  vec2 aliUV = (u_Homography * vec3(refUV, 1.0)).xy;

  vec4 colRef;
  // TODO: Use border clamping for this
  if(refUV.x >= 0.0 && refUV.x <= 1.0 && refUV.y >= 0.0 && refUV.y <= 1.0) {
    colRef = texture(u_RefTexture, refUV) * 10.0;
  } else {
    colRef = vec4(0.0, 0.0, 0.0, 1.0);
  }
  vec4 colAli;// = texture(u_AlignTexture, p_UV) * 10.0;
  if(aliUV.x >= 0.0 && aliUV.x <= 1.0 && aliUV.y >= 0.0 && aliUV.y <= 1.0) {
    colAli = texture(u_AlignTexture, aliUV) * 10.0;
  } else {
    colAli = vec4(0.0, 0.0, 0.0, 1.0);
  }

  vec3 color;// = colRef.xxx;
  switch(u_DisplayType) {
    case 0:
      color = vec3(mix(colRef.x, colAli.x, u_DisplayParam));
      break;
    case 1:
      color = vec3((colRef.x - colAli.x) * u_DisplayParam / 2.0 + 0.5);
      break;
    case 2:
      color = vec3(colRef.x, colAli.x, (colRef.x - colAli.x) * u_DisplayParam / 2.0 + 0.5);
      break;
  }
  o_FragColor = vec4(color, 1.0);
}

