#include "ui/widgets/main_view.hpp"
#include "ui/state.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <cstdlib>
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

  m_mode = RenderMode::DEFAULT;
}

void MainView::resetViewport() {
  // Reset viewport position and scale
  if(m_mode == RenderMode::MATCHES) {
    m_scale = get_width();
    m_offset[0] = 0;
    m_offset[1] = 0;
  } else {
    m_scale = get_width() * 2.0;
    m_offset[0] = -1.0 / m_scale;
    m_offset[1] = -0.5 / m_scale;
  }
}

void MainView::connectState(const std::shared_ptr<UI::State>& state) {
  m_state = state;
  m_images.clear();
  resetViewport();

  // Calculate pixel size from reference image
  auto refSeqImg = m_state->m_sequence->image(m_state->m_sequence->getReferenceImageIndex());
  auto refParams = m_state->m_imageFile.getImageParameters(refSeqImg->getFileIndex());
  m_pixelSize = 1.0 / refParams.width();

  // Construct image views for all images in the sequence
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

  m_keypointCount = 0;
  queue_draw();
}

void MainView::realize() {
  GLAreaPlus::realize();

  // Prepare GL objects
  m_imgProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/main/vert.glsl", "ui/gl/main/frag.glsl"));
  m_selectProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/selector/vert.glsl", "ui/gl/selector/frag.glsl"));
  m_keypointProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/keypoint/vert.glsl", "ui/gl/keypoint/frag.glsl"));
  m_matchProgram = wrap(GL::Program::from_path(get_context(), "ui/gl/match/vert.glsl", "ui/gl/match/frag.glsl"));

  m_dummyVAO = createVertexArray();
  
  m_keypointsVAO = createVertexArray();
  m_keypointsBuffer = createBuffer();
  m_matchesVAO = createVertexArray();
  m_matchesBuffer = createBuffer();

  // Bind buffer to VAO and set the format
  m_keypointsVAO->bind();
  m_keypointsBuffer->bind();

  const size_t vertexSizeKP = 4 * sizeof(float) + 3 * sizeof(uint8_t);
  m_keypointsVAO->attribPointer(0, 2, GL_FLOAT, false, vertexSizeKP, 0);
  m_keypointsVAO->attribPointer(1, 2, GL_FLOAT, false, vertexSizeKP, 2 * sizeof(float));
  m_keypointsVAO->attribPointer(2, 3, GL_UNSIGNED_BYTE, true, vertexSizeKP, 4 * sizeof(float));
  m_keypointsVAO->unbind();

  m_matchesVAO->bind();
  m_matchesBuffer->bind();

  const size_t vertexSizeM = 2 * sizeof(float) + 3 * sizeof(uint8_t);
  m_matchesVAO->attribPointer(0, 2, GL_FLOAT, false, vertexSizeM, 0);
  m_matchesVAO->attribPointer(1, 3, GL_UNSIGNED_BYTE, true, vertexSizeM, 2 * sizeof(float));
  m_matchesVAO->unbind();
}

void MainView::setRenderMode(RenderMode mode) {
  if(m_mode != mode) {
    m_mode = mode;
    resetViewport();
    queue_draw();
  }
}

#define FLAG_DRAW_BORDER 1
#define FLAG_DRAW_UNSELECTED 2

void MainView::calculateViewMatrix() {
  // Recalculate view/projection matrix
  float aspect = (float)get_width() / get_height();
  float scaleFactor = m_scale / get_width();
  memset(m_viewMatrix, 0, sizeof(m_viewMatrix));
  m_viewMatrix[0] = scaleFactor;
  m_viewMatrix[5] = -scaleFactor * aspect;
  m_viewMatrix[10] = 1.0;
  m_viewMatrix[15] = 1.0;

  m_viewMatrix[12] = m_offset[0] * m_scale;
  m_viewMatrix[13] = -m_offset[1] * m_scale;
}

void MainView::renderModeDefault() {
  // Activate image render program
  m_imgProgram->use();

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

  // Put selections on top of images
  renderAllSelections();

  setRenderMode(RenderMode::MATCHES);
}

void MainView::renderAllSelections() {
  // Render selections
  m_dummyVAO->bind();
  m_selectProgram->use();

  // Set shared render parameters
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

  m_dummyVAO->unbind();
}

void MainView::renderModeKeypoints() {
  // Render currently selected image
  auto view = getView(m_sequenceView->getSelectedIndex());
  if(!view)
    return;

  // If the image is marked as dirty we need to rebuild the keypoints mesh
  auto img = view->imageObject();
  if(img->isMarked() || m_keypointCount == 0) {
    rebuildKeypointMesh(img);
  }

  m_imgProgram->use();
  m_imgProgram->uniformMat4fv("u_View", 1, false, m_viewMatrix);
  m_imgProgram->uniform1i("u_Texture", 0);
  m_imgProgram->uniform1i("u_Flags", 0);

  float matrix[9];
  HomographyMatrix::identity(matrix);
  m_imgProgram->uniformMat3fv("u_Transform", 1, false, matrix);

  view->render(*m_imgProgram, false);

  // Draw keypoints
  m_keypointsVAO->bind();
  m_keypointProgram->use();
  m_keypointProgram->uniformMat4fv("u_View", 1, false, m_viewMatrix);

  glDrawArrays(GL_TRIANGLES, 0, m_keypointCount * 6);
  m_keypointsVAO->unbind();
}

void MainView::rebuildKeypointMesh(const Glib::RefPtr<Obj::Image>& image) {
  std::vector<uint8_t> data;
  m_keypointCount = 0;

  auto keypoints = static_cast<std::vector<cv::KeyPoint>*>(image->get_data("keypoints"));
  if(keypoints) {
    for(auto& point : *keypoints) {
      addKeypoint(data, point, 0xFF0000);
    }
  }

  m_keypointsBuffer->bind();
  m_keypointsBuffer->store(data.size(), data.data());
  m_keypointsBuffer->unbind();

  spdlog::debug("Mesh keypoint size = {} bytes for {} keypoints", data.size(), m_keypointCount);
}

void MainView::rebuildMatchMeshes(const Glib::RefPtr<Obj::Image>& reference, const Glib::RefPtr<Obj::Image>& image) {
  std::vector<uint8_t> keypointsData;
  std::vector<uint8_t> matchData;
  m_keypointCount = 0;
  m_matchCount = 0;
  
  auto refKeypoints = static_cast<std::vector<cv::KeyPoint> *>(reference->get_data("keypoints"));
  auto imgKeypoints = static_cast<std::vector<cv::KeyPoint> *>(image->get_data("keypoints"));

  auto imgMatches = static_cast<std::vector<cv::DMatch> *>(image->get_data("matches"));
  if(imgMatches && imgKeypoints && refKeypoints) {
    auto& ref = *refKeypoints;
    auto& img = *imgKeypoints;

    for(auto& match : *imgMatches) {
      auto& ptRef = ref[match.trainIdx];
      auto& ptImg = img[match.queryIdx];

      // Make sure related keypoints have the same color
      uint32_t color = randomColor();
      addKeypoint(keypointsData, ptRef, color, -1.0f);
      addKeypoint(keypointsData, ptImg, color);

      auto addVertex = [&](float x, float y) {
        // Position
        uint8_t *px = reinterpret_cast<uint8_t *>(&x);
        matchData.push_back(px[0]);
        matchData.push_back(px[1]);
        matchData.push_back(px[2]);
        matchData.push_back(px[3]);
        uint8_t *py = reinterpret_cast<uint8_t *>(&y);
        matchData.push_back(py[0]);
        matchData.push_back(py[1]);
        matchData.push_back(py[2]);
        matchData.push_back(py[3]);

        // Color
        matchData.push_back((color >> 16) & 0xFF);
        matchData.push_back((color >>  8) & 0xFF);
        matchData.push_back((color      ) & 0xFF);
      };

      addVertex(ptRef.pt.x * m_pixelSize - 1.0f, ptRef.pt.y * m_pixelSize);
      addVertex(ptImg.pt.x * m_pixelSize, ptImg.pt.y * m_pixelSize);
      ++m_matchCount;
    }
  } else if(refKeypoints) {
    // Render reference keypoints if matches don't exist on the image
    for(auto& point : *refKeypoints) {
      addKeypoint(keypointsData, point, 0xFF0000, -1.0f);
    }
  }
  
  m_keypointsBuffer->bind();
  m_keypointsBuffer->store(keypointsData.size(), keypointsData.data());
  m_keypointsBuffer->unbind();

  m_matchesBuffer->bind();
  m_matchesBuffer->store(matchData.size(), matchData.data());
  m_matchesBuffer->unbind();

  spdlog::debug("Mesh keypoint size = {} bytes for {} keypoints", keypointsData.size(), m_keypointCount);
  spdlog::debug("Mesh matches size = {} bytes for {} matches", matchData.size(), m_matchCount);
}

uint32_t MainView::randomColor() {
  uint32_t v;
  do {
    v = std::rand() & 0xFFFFFF;
  } while(((v >> 16) & 0xFF) < 100 && ((v >> 8) & 0xFF) < 100 && (v & 0xFF) < 100);
  return v;
}

void MainView::addKeypoint(std::vector<uint8_t>& data, const cv::KeyPoint& keypoint, uint32_t rgb, float tX, float tY) {
  auto x = keypoint.pt.x;
  auto y = keypoint.pt.y;

  float angle = keypoint.angle * M_PI / 180.0;
  float sinA = std::sin(angle);
  float cosA = std::cos(angle);

  if(keypoint.angle == -1) {
    sinA = 0;
    cosA = 1;
  }

  auto addData = [&](float value) {
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&value);
    data.push_back(ptr[0]);
    data.push_back(ptr[1]);
    data.push_back(ptr[2]);
    data.push_back(ptr[3]);
  };

  auto addVertex = [&](float offsetX, float offsetY) {
    float oX = (cosA * offsetX + sinA * offsetY) * keypoint.size;
    float oY = (cosA * offsetY - sinA * offsetX) * keypoint.size;
    // Position
    addData((x - oX) * m_pixelSize + tX);
    addData((y + oY) * m_pixelSize + tY);
    // UV
    addData(offsetX);
    addData(offsetY);
    // Color
    data.push_back((rgb >> 16) & 0xFF);
    data.push_back((rgb >>  8) & 0xFF);
    data.push_back((rgb      ) & 0xFF);
  };

  // Triangle 1
  addVertex(-0.5f, -0.5f);
  addVertex(-0.5f,  0.5f);
  addVertex( 0.5f, -0.5f);

  // Triangle 2
  addVertex(-0.5f,  0.5f);
  addVertex( 0.5f, -0.5f);
  addVertex( 0.5f,  0.5f);

  ++m_keypointCount;
}

void MainView::renderModeMatches() {
  // Render currently selected and reference images
  auto imgView = getView(m_sequenceView->getSelectedIndex());
  if(!imgView)
    return;
  auto refView = getView(m_state->m_sequence->getReferenceImageIndex());
  if(!refView)
    return;

  // If any image is marked as dirty we need to rebuild the keypoints mesh
  auto img = imgView->imageObject();
  auto ref = refView->imageObject();
  if(ref->isMarked() || img->isMarked() || m_keypointCount == 0) {
    rebuildMatchMeshes(ref, img);
  }

  // Prepare static program parameters
  m_imgProgram->use();
  m_imgProgram->uniformMat4fv("u_View", 1, false, m_viewMatrix);
  m_imgProgram->uniform1i("u_Texture", 0);
  m_imgProgram->uniform1i("u_Flags", 0);

  // Render selected on the right
  float matrix[9];
  HomographyMatrix::identity(matrix);
  m_imgProgram->uniformMat3fv("u_Transform", 1, true, matrix);
  imgView->render(*m_imgProgram, false);

  // Render reference on the left
  matrix[2] = -1.0f;
  m_imgProgram->uniformMat3fv("u_Transform", 1, true, matrix);
  refView->render(*m_imgProgram, false);

  // Draw keypoints
  m_keypointsVAO->bind();
  m_keypointProgram->use();
  m_keypointProgram->uniformMat4fv("u_View", 1, false, m_viewMatrix);

  glDrawArrays(GL_TRIANGLES, 0, m_keypointCount * 6);
  m_keypointsVAO->unbind();

  // Draw match lines
  m_matchesVAO->bind();
  m_matchProgram->use();
  m_matchProgram->uniformMat4fv("u_View", 1, false, m_viewMatrix);

  glDrawArrays(GL_LINES, 0, m_matchCount * 2);
  m_matchesVAO->unbind();
}

bool MainView::render(const Glib::RefPtr<Gdk::GLContext>& context) {
  spdlog::trace("(MainView) Redraw start");

  // Prepare view matrix
  calculateViewMatrix();

  // Clear buffer
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  switch(m_mode) {
    case RenderMode::DEFAULT:
      renderModeDefault();
      break;
    case RenderMode::KEYPOINTS:
      renderModeKeypoints();
      break;
    case RenderMode::MATCHES:
      renderModeMatches();
      break;
  }

  GLAreaPlus::postRender();

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
    float x = (MODEL_TEMPLATE[i * BUFFER_STRIDE]) * scaleX;
    float y = (MODEL_TEMPLATE[i * BUFFER_STRIDE + 1]) * scaleY;
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

  m_aspect = (double)params.width() / params.height();
  makeVertices(1.0, 1.0 / m_aspect);
}

Glib::RefPtr<Image> ViewImage::imageObject() {
  return m_imageObject;
}

void ViewImage::render(GL::Program& program, bool applyMatrix) {
  if(applyMatrix) {
    float matrix[9];
    if(!m_imageObject->isReference()) {
      auto reg = m_imageObject->getRegistration();
      if(!reg) {
        // Identity matrix
        HomographyMatrix::identity(matrix);
      } else {
        reg->matrix().read(matrix);

        // Scale translations by pixel size
        matrix[2] *=  m_pixelSize;
        matrix[5] *= -m_pixelSize / m_aspect;
      }
    } else {
      // Identity matrix
      HomographyMatrix::identity(matrix);
    }

    program.uniformMat3fv("u_Transform", 1, true, matrix);
  }

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

