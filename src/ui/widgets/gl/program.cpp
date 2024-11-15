#include "ui/widgets/gl/program.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <iostream>
#include <fstream>

#include <spdlog/spdlog.h>

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
    spdlog::error("Shader compilation failed! Message: {}", message);
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
    spdlog::error("Program linking failed! Message: {}", message);
    glDeleteProgram(program);
    program = -1;
  }

  glDeleteShader(vert);
  glDeleteShader(frag);

  m_id = program;
}

Program *Program::from_source(const Glib::RefPtr<Gdk::GLContext>& ctx, const std::string& vert, const std::string& frag) {
  return new Program(ctx, vert.c_str(), frag.c_str());
}

Program *Program::from_stream(const Glib::RefPtr<Gdk::GLContext>& ctx, std::istream& vertStream, std::istream& fragStream) {
  // I might want to use a different approach
  // here since seeking to the end of stream might
  // not be supported on all types of streams.
  vertStream.seekg(0, std::ios::end);
  size_t len = vertStream.tellg();
  vertStream.seekg(0, std::ios::beg);

  std::vector<char> vertBuf(len + 1);
  vertStream.readsome(vertBuf.data(), len);
  vertBuf[len] = 0;

  fragStream.seekg(0, std::ios::end);
  len = fragStream.tellg();
  fragStream.seekg(0, std::ios::beg);

  std::vector<char> fragBuf(len + 1);
  fragStream.readsome(fragBuf.data(), len);
  fragBuf[len] = 0;

  return new Program(ctx, vertBuf.data(), fragBuf.data());
}

Program *Program::from_path(const Glib::RefPtr<Gdk::GLContext>& ctx, const std::filesystem::path& vert, const std::filesystem::path& frag) {
  std::ifstream vertStream(vert);
  if(!vertStream.is_open())
    return nullptr;

  std::ifstream fragStream(frag);
  if(!fragStream.is_open())
    return nullptr;

  return from_stream(ctx, vertStream, fragStream);
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

void Program::uniform1i(const std::string& name, int value) {
  glUniform1i(uniformLocation(name), value);
}

void Program::uniform1f(const std::string& name, float value) {
  glUniform1f(uniformLocation(name), value);
}

void Program::uniform2f(const std::string& name, const float *value) {
  glUniform2f(uniformLocation(name), value[0], value[1]);
}

void Program::uniform3f(const std::string& name, const float *value) {
  glUniform3f(uniformLocation(name), value[0], value[1], value[2]);
}

void Program::uniform4f(const std::string& name, const float *value) {
  glUniform4f(uniformLocation(name), value[0], value[1], value[2], value[3]);
}

void Program::uniform2f(const std::string& name, float x, float y) {
  glUniform2f(uniformLocation(name), x, y);
}

void Program::uniform3f(const std::string& name, float x, float y, float z) {
  glUniform3f(uniformLocation(name), x, y, z);
}

void Program::uniform4f(const std::string& name, float x, float y, float z, float w) {
  glUniform4f(uniformLocation(name), x, y, z, w);
}

void Program::uniformMat3fv(const std::string& name, int count, bool transpose, const float *data) {
  glUniformMatrix3fv(uniformLocation(name), count, transpose ? GL_TRUE : GL_FALSE, data);
}

void Program::uniformMat4fv(const std::string& name, int count, bool transpose, const float *data) {
  glUniformMatrix4fv(uniformLocation(name), count, transpose ? GL_TRUE : GL_FALSE, data);
}

