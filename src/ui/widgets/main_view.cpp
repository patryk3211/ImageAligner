#include "ui/widgets/main_view.hpp"
#include "ui/widgets/gl_area_plus.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <fitsio.h>
#include <iostream>
#include <memory>

using namespace UI;

static const char *VERTEX_SHADER = 
  "#version 150 core\n"
  "in vec2 a_Position;\n"
  "in vec2 a_UV;\n"
  "uniform mat4 u_View;\n"
  "out vec2 p_UV;\n"
  "void main() {\n"
  "  gl_Position = u_View * vec4(a_Position, 0.0, 1.0);\n"
  "  p_UV = a_UV;\n"
  "}\n"
;

static const char *FRAGMENT_SHADER =
  "#version 150 core\n"
  "in vec2 p_UV;\n"
  "out vec4 o_FragColor;\n"
  "uniform sampler2D u_Texture;\n"
  "uniform float u_ColorMult;\n"
  "void main() {\n"
  "  vec4 texCol = texture(u_Texture, p_UV) * u_ColorMult;\n"
  "  o_FragColor = vec4(texCol.xxx, 1.0);\n"
  "}\n"
;

MainView::MainView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : ObjectBase("MainView")
  , GLAreaPlus(cobject, builder) {
  std::cout << "Initializing Main View GL Area" << std::endl;

  m_offset[0] = 0;
  m_offset[1] = 0;
  m_scale = 1;
}

static void GLAPIENTRY messageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam) {
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

void MainView::realize() {
  GLAreaPlus::realize();
  get_context()->make_current();

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(messageCallback, 0);

  m_program = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);

  m_image = new ViewImage(*this);
  m_image->make_vertices(0.5, 0.5, 0);
}

bool MainView::render(const Glib::RefPtr<Gdk::GLContext>& context) {
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
 
  // Prepare vertex buffer format
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (sizeof(float) * 2));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  m_program->use();

  float aspect = (float)get_width() / get_height();
  const float viewMatrix[] = {
    m_scale, 0.0, 0.0, 0.0,
    0.0, m_scale * aspect, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    m_offset[0], m_offset[1], 0.0, 1.0,
  };
  glUniformMatrix4fv(m_program->uniformLocation("u_View"), 1, GL_FALSE, viewMatrix);
  glUniform1i(m_program->uniformLocation("u_Texture"), 0);

  m_image->render(*m_program);

  // Post-render
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);

  // Print all errors that occurred
  GLenum errCode;
  while((errCode = glGetError()) != GL_NO_ERROR) {
    std::cout << "GL Error " << errCode << std::endl;
  }

  return true;
}

ViewImage::ViewImage(GLAreaPlus& area) {
  m_vertices = area.createBuffer();
  m_texture = area.createTexture();
  m_colorMultiplier = 10;
}

// 2 floats for position, 2 for UV
#define BUFFER_STRIDE (2 + 2)

const float MODEL_TEMPLATE[] = {
  -1.0, -1.0,   0.0, 1.0,
  -1.0,  1.0,   0.0, 0.0,
   1.0, -1.0,   1.0, 1.0,
   1.0,  1.0,   1.0, 0.0,
};

void ViewImage::make_vertices(float scaleX, float scaleY, float angle) {
  float buffer[BUFFER_STRIDE * 4];

  for(int i = 0; i < 4; ++i) {
    float x = MODEL_TEMPLATE[i * BUFFER_STRIDE] * scaleX;
    float y = MODEL_TEMPLATE[i * BUFFER_STRIDE + 1] * scaleY;
    // Position
    buffer[i * BUFFER_STRIDE] = std::cos(angle) * x + std::sin(angle) * y;
    buffer[i * BUFFER_STRIDE + 1] = std::cos(angle) * y - std::sin(angle) * x;
    // UV
    buffer[i * BUFFER_STRIDE + 2] = MODEL_TEMPLATE[i * BUFFER_STRIDE + 2];
    buffer[i * BUFFER_STRIDE + 3] = MODEL_TEMPLATE[i * BUFFER_STRIDE + 3];
  }

  m_vertices->store(sizeof(buffer), buffer);
}

void ViewImage::load_texture(Img::Fits& image, int index) {
  image.select(index);

  long dims[2];
  image.imageSize(2, dims);

  long start[3] = { 1, 1, 1 };
  long end[3] = { dims[0], dims[1], 1 };
  long inc[3] = { 1, 1, 1 };

  size_t pixelCount = dims[0] * dims[1] / (inc[0] * inc[1]);
  uint16_t* data = new uint16_t[pixelCount];

  image.readPixelRect(TUSHORT, data, start, end, inc);

  // uint8_t* processedData = new uint8_t[pixelCount];
  // for(size_t i = 0; i < pixelCount; ++i) {
  //   uint8_t value = static_cast<uint8_t>(data[i] / 257);
  //   processedData[i] = value;
  // }

  m_texture->load(dims[0], dims[1], GL_RED, GL_UNSIGNED_SHORT, data, GL_R16);
  make_vertices(1.0, 1.0 * ((float)dims[1] / dims[0]), 0);
}

void ViewImage::render(GL::Program& program) {
  glUniform1f(program.uniformLocation("u_ColorMult"), m_colorMultiplier);

  glActiveTexture(GL_TEXTURE0);
  m_texture->bind();

  m_vertices->bind();

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

