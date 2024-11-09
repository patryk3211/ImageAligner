#include "ui/widgets/main_view.hpp"
#include "sigc++/functors/mem_fun.h"
#include "ui/state.hpp"

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
  "uniform int u_Highlight;\n"
  "void main() {\n"
  "  vec4 texCol = texture(u_Texture, p_UV) * u_ColorMult;\n"
  "  vec2 centerCoords = p_UV * 2.0 - 1.0;\n"
  "  float centDist = max(abs(centerCoords.x), abs(centerCoords.y));\n"
  "  if(u_Highlight > 0 && centDist > 0.95) {\n"
  "    o_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
  "  } else {\n"
  "    o_FragColor = vec4(texCol.xxx, 1.0);\n"
  "  }\n"
  "}\n"
;

MainView::MainView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : ObjectBase("MainView")
  , GLAreaPlus(cobject, builder) {
  std::cout << "Initializing Main View GL Area" << std::endl;

  m_sequenceView = Gtk::Builder::get_widget_derived<SequenceView>(builder, "sequence_view");
  auto selectionModel = m_sequenceView->get_model();
  selectionModel->signal_selection_changed().connect(sigc::mem_fun(*this, &MainView::sequenceViewSelectionChanged));

  auto dragGesture = Gtk::GestureDrag::create();
  dragGesture->signal_drag_begin().connect(sigc::mem_fun(*this, &MainView::dragBegin));
  dragGesture->signal_drag_update().connect(sigc::mem_fun(*this, &MainView::dragUpdate));
  add_controller(dragGesture);

  auto scrollController = Gtk::EventControllerScroll::create();
  scrollController->signal_scroll().connect(sigc::mem_fun(*this, &MainView::scroll), false);
  scrollController->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
  add_controller(scrollController);

  m_offset[0] = 0;
  m_offset[1] = 0;
  m_scale = 1;
}

void MainView::connectState(const std::shared_ptr<UI::State>& state) {
  m_state = state;
  m_images.clear();

  // Reset viewport position
  m_offset[0] = 0;
  m_offset[1] = 0;
  m_scale = 1;

  int x = 0, y = 0;
  for(int i = 0; i < m_state->m_sequence->imageCount(); ++i) {
    auto& imgSeq = m_state->m_sequence->image(i);
    auto imageView = std::make_shared<ViewImage>(*this);
    imageView->m_sequenceImageIndex = i;
    imageView->load_texture(m_state->m_imageFile, imgSeq.m_fileIndex);
    imageView->m_matrix[6] = x * 2;
    imageView->m_matrix[7] = -y * 2;
    if(++x > 5) {
      x = 0;
      ++y;
    }
    m_images.push_back(imageView);
  }

  queue_draw();
}

void MainView::sequenceViewSelectionChanged(uint position, uint nitems) {
  auto selectionModel = m_sequenceView->get_model();
  auto& model = dynamic_cast<Gio::ListModel&>(*selectionModel);

  int selectedIndex = -1;
  for(uint i = position; i < model.get_n_items(); ++i) {
    if(selectionModel->is_selected(i)) {
      selectedIndex = i;
      break;
    }
  }

  if(selectedIndex == -1)
    return;

  // Extract selected image object
  auto iter = m_images.begin();
  while(iter != m_images.end()) {
    if((*iter)->m_sequenceImageIndex == selectedIndex)
      break;
    ++iter;
  }

  // Image not found
  if(iter == m_images.end()) {
    std::cout << "Selected image not found in view images" << std::endl;
    return;
  }

  // Remove image from list...
  auto selectedImage = *iter;
  m_images.erase(iter);
  // ...and put it in the beginning
  m_images.push_front(selectedImage);

  queue_draw();
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
    m_offset[0] * m_scale, m_offset[1] * m_scale, 0.0, 1.0,
  };
  m_program->uniformMat4fv("u_View", 1, false, viewMatrix);
  m_program->uniform1i("u_Texture", 0);

  for(auto iter = m_images.rbegin(); iter != m_images.rend(); ++iter) {
    auto image = *iter;
    if(image == m_images.front()) {
      m_program->uniform1i("u_Highlight", 1);
    } else {
      m_program->uniform1i("u_Highlight", 0);
    }
    image->render(*m_program);
  }

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

void MainView::dragBegin(double startX, double startY) {
  m_savedOffset[0] = m_offset[0];
  m_savedOffset[1] = m_offset[1];
}

void MainView::dragUpdate(double offsetX, double offsetY) {
  m_offset[0] = m_savedOffset[0] + offsetX / get_width() * 2 / m_scale;
  m_offset[1] = m_savedOffset[1] - offsetY / get_height() * 2 / m_scale;
  queue_draw();
}

bool MainView::scroll(double x, double y) {
  if(y > 0) {
    m_scale *= 0.5;
  } else {
    m_scale *= 2.0;
  }

  queue_draw();
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
  auto params = image.getImageParameters(index);
  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  // Make large images use a subset of pixels to save VRAM
  int scale = params.width() / 500;
  if(scale < 0) scale = 1;
  params.setDimension(0, -1, -1, scale);
  params.setDimension(1, -1, -1, scale);

  if(params.type() != Img::DataType::SHORT && params.type() != Img::DataType::USHORT) {
    std::cerr << "Not a short type 16 bit image" << std::endl;
    return;
  }

  auto data = image.getPixels(params);
  m_texture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);
  make_vertices(1.0, 1.0 * ((float)params.height() / params.width()));
}

void ViewImage::render(GL::Program& program) {
  program.uniform1f("u_ColorMult", m_colorMultiplier);
  program.uniformMat3fv("u_Transform", 1, false, m_matrix);

  glActiveTexture(GL_TEXTURE0);
  m_texture->bind();

  m_vertices->bind();

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

