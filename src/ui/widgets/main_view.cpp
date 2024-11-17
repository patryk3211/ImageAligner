#include "ui/widgets/main_view.hpp"
#include "ui/state.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <fitsio.h>
#include <spdlog/spdlog.h>

using namespace UI;
using namespace Obj;
using namespace IO;

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

  Glib::Binding::bind_property(m_minLevelBtn->property_adjustment(), m_minLevelScale->property_adjustment(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
  Glib::Binding::bind_property(m_maxLevelBtn->property_adjustment(), m_maxLevelScale->property_adjustment(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);

  m_offset[0] = 0;
  m_offset[1] = 0;
  m_scale = 1;

  m_pixelSize = 1;

  memset(m_currentSelection, 0, sizeof(m_currentSelection));
  m_makeSelection = false;
}

void MainView::connectState(const std::shared_ptr<UI::State>& state) {
  m_state = state;
  m_images.clear();

  // Reset viewport position
  m_offset[0] = 0;
  m_offset[1] = 0;
  m_scale = get_width();

  auto refSeqImg = m_state->m_sequence->image(m_state->m_sequence->getReferenceImageIndex());
  auto refParams = m_state->m_imageFile.getImageParameters(refSeqImg->getFileIndex());
  m_pixelSize = 1.0 / refParams.width();

  int x = 0, y = 0;
  for(int i = 0; i < m_state->m_sequence->getImageCount(); ++i) {
    auto imageView = std::make_shared<ViewImage>(*this, m_sequenceView->getImage(i));
    imageView->imageObject()->signalRedraw().connect(sigc::mem_fun(*this, &MainView::queue_draw));
    m_images.push_back(imageView);
  }

  sequenceViewSelectionChanged(0, 0);
}

std::shared_ptr<ViewImage> MainView::getView(int seqIndex) {
  for(auto& view : m_images) {
    if(view->imageObject()->getSequenceIndex() == seqIndex)
      return view;
  }
  return nullptr;
}

void MainView::requestSelection(const selection_callback& callback, float forceAspect) {
  memset(m_currentSelection, 0, sizeof(m_currentSelection));
  m_selectCallback = callback;
  m_makeSelection = true;
}

void MainView::sequenceViewSelectionChanged(uint position, uint nitems) {
  uint selectedIndex = m_sequenceView->getSelectedIndex();

  // Extract selected image object
  auto iter = m_images.begin();
  while(iter != m_images.end()) {
    if((*iter)->imageObject()->getSequenceIndex() == selectedIndex)
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

  // Change level adjuster bindings
  for(int i = 0; i < 2; ++i) {
    if(m_levelBindings[i] != nullptr) {
      m_levelBindings[i]->unbind();
      m_levelBindings[i] = nullptr;
    }
  }
 
  auto imgObj = selectedImage->imageObject();

  auto minAdj = m_minLevelBtn->get_adjustment();
  auto maxAdj = m_maxLevelBtn->get_adjustment();
  minAdj->set_lower(0);
  minAdj->set_upper(m_state->m_imageFile.maxTypeValue());
  maxAdj->set_lower(0);
  maxAdj->set_upper(m_state->m_imageFile.maxTypeValue());

  auto stats = imgObj->getStats(0);
  if(!stats) {
    imgObj->calculateStats(m_state->m_imageFile);
    stats = imgObj->getStats(0);
  }
  m_levelBindings[0] = Glib::Binding::bind_property(stats->propertyMin(), m_minLevelBtn->property_value(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
  m_levelBindings[1] = Glib::Binding::bind_property(stats->propertyMax(), m_maxLevelBtn->property_value(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);

  queue_draw();
}

void MainView::realize() {
  GLAreaPlus::realize();

  m_imgProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/main/vert.glsl", "ui/gl/main/frag.glsl"));
  m_selectProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/selector/vert.glsl", "ui/gl/selector/frag.glsl"));
}

#define FLAG_DRAW_BORDER 1
#define FLAG_DRAW_UNSELECTED 2

bool MainView::render(const Glib::RefPtr<Gdk::GLContext>& context) {
  spdlog::trace("(MainView) Redraw start");

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  // Activate image render program
  m_imgProgram->use();

  // Recalculate view/projection matrix
  float aspect = (float)get_width() / get_height();
  float scaleFactor = m_scale / get_width();
  memset(m_viewMatrix, 0, sizeof(m_viewMatrix));
  m_viewMatrix[0] = scaleFactor;
  m_viewMatrix[5] = -scaleFactor * aspect;
  m_viewMatrix[10] = 1.0;
  m_viewMatrix[12] = m_offset[0] * m_scale;
  m_viewMatrix[13] = -m_offset[1] * m_scale;
  m_viewMatrix[15] = 1.0;
  // {
  //   scaleFactor, 0.0, 0.0, 0.0,
  //   0.0, -scaleFactor * aspect, 0.0, 0.0,
  //   0.0, 0.0, 1.0, 0.0,
  //   m_offset[0] * m_scale, -m_offset[1] * m_scale, 0.0, 1.0,
  // };

  m_imgProgram->uniformMat4fv("u_View", 1, false, m_viewMatrix);
  m_imgProgram->uniform1i("u_Texture", 0);

  // Render all images
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

  // Render selections
  m_selectProgram->use();
  m_selectProgram->uniformMat4fv("u_View", 1, false, m_viewMatrix);
  m_selectProgram->uniform1f("u_Scale", m_scale / get_width());
  m_selectProgram->uniform1f("u_Width", get_width());

  if(m_makeSelection) {
    // Render the selection being currently made
    renderSelection(m_currentSelection[0], m_currentSelection[1], m_currentSelection[2], m_currentSelection[3]);
  }

  // Render all other active selections
  for(auto& sel : m_selections) {
    renderSelection(sel->m_x, sel->m_y, sel->m_width, sel->m_height);
  }

  // Print all errors that occurred
  GLenum errCode;
  while((errCode = glGetError()) != GL_NO_ERROR) {
    spdlog::error("GL Error {}", errCode);
  }

  spdlog::trace("(MainView) Redraw end");
  return true;
}

Selection::Selection(MainView& view, float x, float y, float width, float height)
  : m_view(view)
  , m_x(x)
  , m_y(y)
  , m_width(width)
  , m_height(height) {
  m_view.m_selections.push_back(this);
}

Selection::~Selection() {
  m_view.m_selections.remove(this);
}

void MainView::renderSelection(float x, float y, float width, float height) {
  m_selectProgram->uniform4f("u_Selection", x, y, width, height);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void MainView::dragBegin(double startX, double startY) {
  spdlog::trace("(MainView) Drag begin");

  if(!m_makeSelection) {
    m_savedOffset[0] = m_offset[0];
    m_savedOffset[1] = m_offset[1];
  } else {
    // These translations are absolutely insane
    m_currentSelection[0] = (startX * 2 - (float)get_width()) / m_scale - m_offset[0] * get_width();
    m_currentSelection[1] = (startY * 2 - (float)get_height()) / m_scale - m_offset[1] * get_height();
  }
}

void MainView::dragUpdate(double offsetX, double offsetY) {
  if(!m_makeSelection) {
    m_offset[0] = m_savedOffset[0] + offsetX / get_width() * 2 / m_scale;
    m_offset[1] = m_savedOffset[1] + offsetY / get_height() * 2 / m_scale;
  } else {
    m_currentSelection[2] = offsetX * 2 / m_scale;
    m_currentSelection[3] = offsetY * 2 / m_scale;
  }
  queue_draw();
}

void MainView::dragEnd(double endX, double endY) {
  spdlog::trace("(MainView) Drag end");

  if(m_currentSelection[2] < 0) {
    m_currentSelection[0] += m_currentSelection[2];
    m_currentSelection[2] = -m_currentSelection[2];
  }
  if(m_currentSelection[3] < 0) {
    m_currentSelection[1] += m_currentSelection[3];
    m_currentSelection[3] = -m_currentSelection[3];
  }
  if(m_selectCallback) {
    auto selection = std::make_shared<Selection>(*this, m_currentSelection[0], m_currentSelection[1], m_currentSelection[2], m_currentSelection[3]);
    (*m_selectCallback)(selection);
    m_selectCallback = std::nullopt;
  }
  m_makeSelection = false;
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

double MainView::pixelSize() const {
  return m_pixelSize;
}

std::shared_ptr<UI::State> MainView::state() {
  return m_state;
}

ViewImage::ViewImage(MainView& area, const Glib::RefPtr<Image>& image)
  : m_imageObject(image) {
  m_vertices = area.createBuffer();
  m_texture = area.createTexture();
  m_vao = area.createVertexArray();

  m_vao->bind();
  m_vertices->bind();
  m_vao->attribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(float), 0);
  m_vao->attribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float));

  loadTexture(area.state()->m_imageFile, image->getSequenceIndex());

  m_vao->unbind();

  m_pixelSize = area.pixelSize();
  m_maxValue = area.state()->m_imageFile.maxTypeValue();
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

void ViewImage::loadTexture(ImageProvider& image, int index) {
  auto params = image.getImageParameters(index);
  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  // Make large images use a subset of pixels to save VRAM
  int scale = params.width() / 500;
  if(scale < 0) scale = 1;
  params.setDimension(0, -1, -1, scale);
  params.setDimension(1, -1, -1, scale);

  if(params.type() != DataType::SHORT && params.type() != DataType::USHORT) {
    spdlog::error("Not a short type 16 bit image");
    return;
  }

  auto data = image.getPixels(params);
  m_texture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);
  makeVertices(1.0, 1.0 * ((float)params.height() / params.width()));
}

Glib::RefPtr<Image> ViewImage::imageObject() {
  return m_imageObject;
}

void ViewImage::render(GL::Program& program) {
  float matrix[9];
  if(!m_imageObject->isReference()) {
    auto reg = m_imageObject->getRegistration();
    if(!reg) {
      // Identity matrix
      HomographyMatrix::identity(matrix);
    } else {
      reg->matrix().read(matrix);
    }

    // Scale translations by pixel size
    matrix[2] *=  m_pixelSize;
    matrix[5] *= -m_pixelSize;
  } else {
    // Identity matrix
    HomographyMatrix::identity(matrix);
  }

  program.uniformMat3fv("u_Transform", 1, true, matrix);

  auto stats = m_imageObject->getStats(0);
  if(stats) {
    float min = stats->getMin() / m_maxValue;
    float max = stats->getMax() / m_maxValue;
    program.uniform2f("u_Levels", min, max);
  } else {
    program.uniform2f("u_Levels", 0, 1);
  }

  glActiveTexture(GL_TEXTURE0);
  m_texture->bind();
  m_vao->bind();

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  m_vao->unbind();
  m_texture->unbind();

  m_imageObject->clearRedrawFlag();
}

