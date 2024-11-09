#include "ui/widgets/main_view.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <fitsio.h>
#include <iostream>

using namespace UI;

static const char *VERTEX_SHADER = 
  "#version 150 core\n"
  "in vec2 a_Position;\n"
  "in vec2 a_UV;\n"
  "uniform mat4 u_View;\n"
  "uniform mat3 u_Transform;\n"
  "out vec2 p_UV;\n"
  "void main() {\n"
  "  vec3 imgPos = u_Transform * vec3(a_Position, 1.0);\n"
  "  gl_Position = u_View * vec4(imgPos.xy, 0.0, 1.0);\n"
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
  m_image->make_vertices(0.5, 0.5);
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

  memset(m_matrix, 0, sizeof(m_matrix));
  m_matrix[0] = 1;
  m_matrix[4] = 1;
  m_matrix[8] = 1;
}

// 2 floats for position, 2 for UV
#define BUFFER_STRIDE (2 + 2)

const float MODEL_TEMPLATE[] = {
  -1.0, -1.0,   0.0, 1.0,
  -1.0,  1.0,   0.0, 0.0,
   1.0, -1.0,   1.0, 1.0,
   1.0,  1.0,   1.0, 0.0,
};

void ViewImage::make_vertices(float scaleX, float scaleY) {
  float buffer[BUFFER_STRIDE * 4];

  for(int i = 0; i < 4; ++i) {
    float x = MODEL_TEMPLATE[i * BUFFER_STRIDE] * scaleX;
    float y = MODEL_TEMPLATE[i * BUFFER_STRIDE + 1] * scaleY;
    // Position
    buffer[i * BUFFER_STRIDE] = x;
    buffer[i * BUFFER_STRIDE + 1] = y;
    // UV
    buffer[i * BUFFER_STRIDE + 2] = MODEL_TEMPLATE[i * BUFFER_STRIDE + 2];
    buffer[i * BUFFER_STRIDE + 3] = MODEL_TEMPLATE[i * BUFFER_STRIDE + 3];
  }

  m_vertices->store(sizeof(buffer), buffer);
}

void ViewImage::load_texture(Img::ImageProvider& image, int index) {
  auto params = image.getImageParameters(0);
  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  if(params.type() != Img::DataType::SHORT && params.type() != Img::DataType::USHORT) {
    std::cerr << "Not a short type 16 bit image" << std::endl;
    return;
  }

  auto data = image.getPixels(params);

  m_texture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);
  make_vertices(1.0, 1.0 * ((float)params.height() / params.width()));
}

void ViewImage::render(GL::Program& program) {
  glUniform1f(program.uniformLocation("u_ColorMult"), m_colorMultiplier);
  glUniformMatrix3fv(program.uniformLocation("u_Transform"), 1, GL_FALSE, m_matrix);

  glActiveTexture(GL_TEXTURE0);
  m_texture->bind();

  m_vertices->bind();

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

