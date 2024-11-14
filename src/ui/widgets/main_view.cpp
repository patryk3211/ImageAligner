#include "ui/widgets/main_view.hpp"
#include "ui/state.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <fitsio.h>
#include <spdlog/spdlog.h>

using namespace UI;

MainView::MainView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : ObjectBase("MainView")
  , GLAreaPlus(cobject, builder) {
  spdlog::info("Initializing Main View GL Area");

  m_sequenceView = Gtk::Builder::get_widget_derived<SequenceView>(builder, "sequence_view");
  auto selectionModel = m_sequenceView->get_model();
  selectionModel->signal_selection_changed().connect(sigc::mem_fun(*this, &MainView::sequenceViewSelectionChanged));

  auto dragGesture = Gtk::GestureDrag::create();
  dragGesture->signal_drag_begin().connect(sigc::mem_fun(*this, &MainView::dragBegin));
  dragGesture->signal_drag_update().connect(sigc::mem_fun(*this, &MainView::dragUpdate));
  dragGesture->signal_drag_end().connect(sigc::mem_fun(*this, &MainView::dragEnd));
  add_controller(dragGesture);

  auto scrollController = Gtk::EventControllerScroll::create();
  scrollController->signal_scroll().connect(sigc::mem_fun(*this, &MainView::scroll), false);
  scrollController->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
  add_controller(scrollController);

  m_hideUnselected = builder->get_widget<Gtk::CheckButton>("show_only_selected_btn");
  m_hideUnselected->signal_toggled().connect(sigc::mem_fun(*this, &MainView::queue_draw));

  m_minLevelBtn = builder->get_widget<Gtk::SpinButton>("level_min_btn");
  m_maxLevelBtn = builder->get_widget<Gtk::SpinButton>("level_max_btn");
  m_minLevelScale = builder->get_widget<Gtk::Scale>("level_min_scale");
  m_maxLevelScale = builder->get_widget<Gtk::Scale>("level_max_scale");

  m_offset[0] = 0;
  m_offset[1] = 0;
  m_scale = 1;

  m_pixelSize = 1;

  memset(m_selection, 0, sizeof(m_selection));
  m_makeSelection = false;
}

void MainView::connectState(const std::shared_ptr<UI::State>& state) {
  m_state = state;
  m_images.clear();

  // Reset viewport position
  m_offset[0] = 0;
  m_offset[1] = 0;
  m_scale = get_width();

  auto& refSeqImg = m_state->m_sequence->image(m_state->m_sequence->referenceImage());
  auto refParams = m_state->m_imageFile.getImageParameters(refSeqImg.m_fileIndex);
  m_pixelSize = 1.0 / refParams.width();

  int x = 0, y = 0;
  for(int i = 0; i < m_state->m_sequence->imageCount(); ++i) {
    auto imageView = std::make_shared<ViewImage>(*this, m_sequenceView->getImage(i));
    m_images.push_back(imageView);
  }

  queue_draw();
}

std::shared_ptr<ViewImage> MainView::getView(int seqIndex) {
  for(auto& view : m_images) {
    if(view->imageObject()->getIndex() == seqIndex)
      return view;
  }
  return nullptr;
}

void MainView::refreshRegistration(int index) {
  auto view = getView(index);
  if(index == m_state->m_sequence->referenceImage()) {
    view->resetHomography();
  } else {
    auto seqImg = m_state->m_sequence->image(index);
    if(seqImg.m_registration) {
      view->registrationHomography(seqImg.m_registration->m_homographyMatrix, m_pixelSize);
    }
  }
  queue_draw();
}

void MainView::requestSelection(const std::function<void(float, float, float, float)>& callback, float forceAspect) {
  memset(m_selection, 0, sizeof(m_selection));
  m_selectCallback = callback;
  m_makeSelection = true;
}

void MainView::sequenceViewSelectionChanged(uint position, uint nitems) {
  uint selectedIndex = m_sequenceView->getSelectedIndex();

  // Extract selected image object
  auto iter = m_images.begin();
  while(iter != m_images.end()) {
    if((*iter)->imageObject()->getIndex() == selectedIndex)
      break;
    ++iter;
  }

  // Image not found
  if(iter == m_images.end()) {
    spdlog::warn("Selected image not found in view images");
    return;
  }

  // Remove image from list...
  auto selectedImage = *iter;
  m_images.erase(iter);
  // ...and put it in the beginning
  m_images.push_front(selectedImage);

  queue_draw();
}

void MainView::realize() {
  GLAreaPlus::realize();

  m_imgProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/main/vert.glsl", "ui/gl/main/frag.glsl"));
  m_selectProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/selector/vert.glsl", "ui/gl/selector/frag.glsl"));
}

void MainView::enableVertexAttribs() { 
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (sizeof(float) * 2));
}

void MainView::disableVertexAttribs() {
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
}

#define FLAG_DRAW_BORDER 1
#define FLAG_DRAW_UNSELECTED 2

bool MainView::render(const Glib::RefPtr<Gdk::GLContext>& context) {
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  // Prepare vertex buffer format
  enableVertexAttribs();

  m_imgProgram->use();

  float aspect = (float)get_width() / get_height();
  float scaleFactor = m_scale / get_width();
  const float viewMatrix[] = {
    scaleFactor, 0.0, 0.0, 0.0,
    0.0, -scaleFactor * aspect, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    m_offset[0] * m_scale, -m_offset[1] * m_scale, 0.0, 1.0,
  };
  m_imgProgram->uniformMat4fv("u_View", 1, false, viewMatrix);
  m_imgProgram->uniform1i("u_Texture", 0);

  bool hideUnselected = m_hideUnselected->get_active();
  for(auto iter = m_images.rbegin(); iter != m_images.rend(); ++iter) {
    auto& image = *iter;
    int flags = 0;
    if(!image->imageObject()->getIncluded()) {
      if(hideUnselected)
        continue;
      // Indicate that the image is not selected
      flags |= FLAG_DRAW_UNSELECTED;
    }
    flags |= image == m_images.front() ? FLAG_DRAW_BORDER : 0;
    m_imgProgram->uniform1i("u_Flags", flags);
    image->render(*m_imgProgram);
  }

  // Post-render
  disableVertexAttribs();

  // Render selection
  m_selectProgram->use();
  m_selectProgram->uniformMat4fv("u_View", 1, false, viewMatrix);
  m_selectProgram->uniform4f("u_Selection", m_selection);
  m_selectProgram->uniform1f("u_Scale", scaleFactor);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Print all errors that occurred
  GLenum errCode;
  while((errCode = glGetError()) != GL_NO_ERROR) {
    spdlog::error("GL Error {}", errCode);
  }

  return true;
}

void MainView::dragBegin(double startX, double startY) {
  if(!m_makeSelection) {
    m_savedOffset[0] = m_offset[0];
    m_savedOffset[1] = m_offset[1];
  } else {
    // These translations are absolutely insane
    m_selection[0] = (startX * 2 - (float)get_width()) / m_scale - m_offset[0] * get_width();
    m_selection[1] = (startY * 2 - (float)get_height()) / m_scale - m_offset[1] * get_height();
  }
}

void MainView::dragUpdate(double offsetX, double offsetY) {
  if(!m_makeSelection) {
    m_offset[0] = m_savedOffset[0] + offsetX / get_width() * 2 / m_scale;
    m_offset[1] = m_savedOffset[1] + offsetY / get_height() * 2 / m_scale;
  } else {
    m_selection[2] = offsetX * 2 / m_scale;
    m_selection[3] = offsetY * 2 / m_scale;
  }
  queue_draw();
}

void MainView::dragEnd(double endX, double endY) {
  if(m_selection[2] < 0) {
    m_selection[0] += m_selection[2];
    m_selection[2] = -m_selection[2];
  }
  if(m_selection[3] < 0) {
    m_selection[1] += m_selection[3];
    m_selection[3] = -m_selection[3];
  }
  if(m_selectCallback) {
    (*m_selectCallback)(m_selection[0], m_selection[1], m_selection[2], m_selection[3]);
    m_selectCallback = std::nullopt;
  }
  m_makeSelection = false;
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

double MainView::pixelSize() const {
  return m_pixelSize;
}

ViewImage::ViewImage(MainView& area, const Glib::RefPtr<Image>& image)
  : m_imageObject(image) {
  m_vertices = area.createBuffer();
  m_texture = area.createTexture();

  loadTexture(image->getProvider(), image->getIndex());
  resetHomography();

  auto& seqImg = image->getSequenceImage();
  if(seqImg.m_registration && !image->isReference()) {
    registrationHomography(seqImg.m_registration->m_homographyMatrix, area.pixelSize());
  }
}

// 2 floats for position, 2 for UV
#define BUFFER_STRIDE (2 + 2)

const float MODEL_TEMPLATE[] = {
  0.0, 0.0,   0.0, 0.0,
  0.0, 1.0,   0.0, 1.0,
  1.0, 0.0,   1.0, 0.0,
  1.0, 1.0,   1.0, 1.0,
};

void ViewImage::makeVertices(float scaleX, float scaleY) {
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

void ViewImage::loadTexture(Img::ImageProvider& image, int index) {
  auto params = image.getImageParameters(index);
  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  // Make large images use a subset of pixels to save VRAM
  int scale = params.width() / 500;
  if(scale < 0) scale = 1;
  params.setDimension(0, -1, -1, scale);
  params.setDimension(1, -1, -1, scale);

  if(params.type() != Img::DataType::SHORT && params.type() != Img::DataType::USHORT) {
    spdlog::error("Not a short type 16 bit image");
    return;
  }

  auto data = image.getPixels(params);
  m_texture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);
  makeVertices(1.0, 1.0 * ((float)params.height() / params.width()));
}

void ViewImage::registrationHomography(const double *homography, double refPixelSize) {
  m_matrix[0] = homography[0];
  m_matrix[3] = homography[1];
  m_matrix[6] = homography[2] * refPixelSize;

  m_matrix[1] = homography[3];
  m_matrix[4] = homography[4];
  m_matrix[7] = -homography[5] * refPixelSize;

  m_matrix[2] = homography[6];
  m_matrix[5] = homography[7];
  m_matrix[8] = homography[8];
}

void ViewImage::resetHomography() {
  memset(m_matrix, 0, sizeof(m_matrix));
  m_matrix[0] = 1;
  m_matrix[4] = 1;
  m_matrix[8] = 1;
}

Glib::RefPtr<Image> ViewImage::imageObject() {
  return m_imageObject;
}

void ViewImage::render(GL::Program& program) {
  program.uniformMat3fv("u_Transform", 1, false, m_matrix);

  float min = m_imageObject->getScaledMinLevel();
  float max = m_imageObject->getScaledMaxLevel();
  program.uniform2f("u_Levels", min, max);

  glActiveTexture(GL_TEXTURE0);
  m_texture->bind();

  m_vertices->bind();

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

