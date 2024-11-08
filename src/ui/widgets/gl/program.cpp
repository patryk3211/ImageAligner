#include "ui/widgets/gl/program.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <iostream>

using namespace UI::GL;

static uint shader(uint type, const char* source) {
  uint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, 0);
  glCompileShader(shader);

  int status;
  char message[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(!status) {
    glGetShaderInfoLog(shader, 512, 0, message);
    std::cerr << "Shader compilation failed! Message: " << message << std::endl;
    glDeleteShader(shader);
    return -1;
  }

  return shader;
}

Program::Program(const Glib::RefPtr<Gdk::GLContext>& ctx, const char* vertSource, const char* fragSource)
  : Object(ctx) {
  uint vert = shader(GL_VERTEX_SHADER, vertSource);
  if(vert == -1) {
    m_id = -1;
    return;
  }
  uint frag = shader(GL_FRAGMENT_SHADER, fragSource);
  if(frag == -1) {
    m_id = -1;
    glDeleteShader(vert);
    return;
  }

  uint program = glCreateProgram();
  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);

  int status;
  char message[512];
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if(!status) {
    glGetProgramInfoLog(program, 512, 0, message);
    std::cerr << "Program linking failed! Message: " << message << std::endl;
    glDeleteProgram(program);
    program = -1;
  }

  glDeleteShader(vert);
  glDeleteShader(frag);

  m_id = program;
}

Program::~Program() {
  destroy();
}

void Program::destroy() {
  prepare_context();
  if(m_id != NULL_ID) {
    glDeleteProgram(m_id);
    m_id = NULL_ID;
  }
}

void Program::use() {
  glUseProgram(m_id);
}

uint Program::uniformLocation(const std::string& name) {
  prepare_context();

  auto iter = m_locations.find(name);
  if(iter == m_locations.end()) {
    uint loc = glGetUniformLocation(m_id, name.c_str());
    m_locations.insert({ name, loc });
    return loc;
  }

  return iter->second;
}

