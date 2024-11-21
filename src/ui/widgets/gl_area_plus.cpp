#include "ui/widgets/gl_area_plus.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <spdlog/spdlog.h>

using namespace UI;

// GLAreaPlus *GLAreaPlus::s_contextHolder = 0;

GLAreaPlus::GLAreaPlus(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : GLArea(cobject) {
  signal_create_context().connect(sigc::mem_fun(*this, &GLAreaPlus::createContext), false);
  signal_realize().connect(sigc::mem_fun(*this, &GLAreaPlus::realize), false);
  signal_render().connect(sigc::mem_fun(*this, &GLAreaPlus::render), false);
  signal_unrealize().connect(sigc::mem_fun(*this, &GLAreaPlus::unrealize), false);
}

GLAreaPlus::~GLAreaPlus() {
  // if(s_contextHolder == this)
  //   s_contextHolder = 0;
}

static void GLAPIENTRY messageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam) {
  const char *typeStr = 0;
  switch(type) {
    case GL_DEBUG_TYPE_ERROR: typeStr = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecation"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "Undefined"; break;
    case GL_DEBUG_TYPE_PORTABILITY: typeStr = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: typeStr = "Performance"; break;
    case GL_DEBUG_TYPE_MARKER: typeStr = "Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP: typeStr = "GroupPush"; break;
    case GL_DEBUG_TYPE_POP_GROUP: typeStr = "GroupPop"; break;
    case GL_DEBUG_TYPE_OTHER: typeStr = "Other"; break;
    default: typeStr = "Unknown"; break;
  }

  const char *sourceStr = 0;
  switch(source) {
    case GL_DEBUG_SOURCE_API: sourceStr = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "WindowSystem"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: sourceStr = "ThirdParty"; break;
    case GL_DEBUG_SOURCE_APPLICATION: sourceStr = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER: sourceStr = "Other"; break;
    default: sourceStr = "Unknown"; break;
  }

  if(severity == GL_DEBUG_SEVERITY_HIGH) {
    spdlog::error("GL Message [{}@{}]: {}", typeStr, sourceStr, message);
  } else if(severity == GL_DEBUG_SEVERITY_MEDIUM) {
    spdlog::warn("GL Message [{}@{}]: {}", typeStr, sourceStr, message);
  } else if(severity == GL_DEBUG_SEVERITY_LOW) {
    spdlog::info("GL Message [{}@{}]: {}", typeStr, sourceStr, message);
  } else {
    spdlog::debug("GL Message [{}@{}]: {}", typeStr, sourceStr, message);
  }
}

std::shared_ptr<GL::Texture> GLAreaPlus::createTexture() {
  return wrap(new GL::Texture(get_context()));
}

std::shared_ptr<GL::Buffer> GLAreaPlus::createBuffer() {
  return wrap(new GL::Buffer(get_context()));
}

std::shared_ptr<GL::Program> GLAreaPlus::createProgram(const char *vert, const char *frag) {
  return wrap(new GL::Program(get_context(), vert, frag));
}

std::shared_ptr<GL::VAO> GLAreaPlus::createVertexArray() {
  return wrap(new GL::VAO(get_context()));
}

Glib::RefPtr<Gdk::GLContext> GLAreaPlus::createContext() {
  // if(s_contextHolder != 0)
  //   return s_contextHolder->get_context();

  auto context = get_native()->get_surface()->create_gl_context();
  context->set_allowed_apis(Gdk::GLApi::GL | Gdk::GLApi::GLES);
  context->set_required_version(3, 2);
  context->realize();

  spdlog::debug("New GL context created");

  context->make_current();
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(messageCallback, 0);

  // s_contextHolder = this;
  return context;
}

void GLAreaPlus::realize() {
  auto ctx = get_context();
  ctx->make_current();

  int major, minor;
  ctx->get_version(major, minor);

  const char* apiStr = 0;
  switch(ctx->get_api2()) {
    case Gdk::GLApi::GL:
      apiStr = "OpenGL";
      break;
    case Gdk::GLApi::GLES:
      apiStr = "OpenGL ES";
      break;
    default:
      apiStr = "NOAPI";
      break;
  }

  spdlog::debug("{} context version {}.{}", apiStr, major, minor);
}

void GLAreaPlus::unrealize() {
  // Delete all allocated GL objects
  for(auto& obj : m_objects) {
    if(obj.expired())
      continue;
    auto locked = obj.lock();
    locked->destroy();
  }
}

void GLAreaPlus::postRender() {
  // Print all errors that occurred
  GLenum errCode;
  while((errCode = glGetError()) != GL_NO_ERROR) {
    spdlog::error("GL Error {}", errCode);
  }
}

