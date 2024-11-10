#include "ui/widgets/alignment_view.hpp"
#include "ui/state.hpp"

#include <GL/gl.h>
#include <spdlog/spdlog.h>

using namespace UI;

AlignmentView::AlignmentView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : GLAreaPlus(cobject, builder) {
  m_refImgBtn = builder->get_widget<Gtk::SpinButton>("ref_image_spin_btn");
  m_refImgBtn->signal_value_changed().connect(sigc::mem_fun(*this, &AlignmentView::referenceChanged));

  m_sequenceView = Gtk::Builder::get_widget_derived<SequenceView>(builder, "sequence_view");
  auto selectionModel = m_sequenceView->get_model();
  selectionModel->signal_selection_changed().connect(sigc::mem_fun(*this, &AlignmentView::sequenceViewSelectionChanged));

  m_mainView = Gtk::Builder::get_widget_derived<MainView>(builder, "main_gl_area");

  auto prevImg = builder->get_widget<Gtk::Button>("align_prev_img");
  prevImg->signal_clicked().connect(sigc::mem_fun(*m_sequenceView, &SequenceView::prevImage));
  auto nextImg = builder->get_widget<Gtk::Button>("align_next_img");
  nextImg->signal_clicked().connect(sigc::mem_fun(*m_sequenceView, &SequenceView::nextImage));

  auto alignPickArea = builder->get_widget<Gtk::Button>("align_pick_area");
  alignPickArea->signal_clicked().connect(sigc::mem_fun(*this, &AlignmentView::pickArea));

  m_viewTypeBtn = builder->get_widget<Gtk::SpinButton>("align_view_display_type");
  m_viewTypeBtn->signal_value_changed().connect(sigc::mem_fun(*this, &AlignmentView::viewTypeChanged));
  m_viewParamBtn = builder->get_widget<Gtk::SpinButton>("align_view_display_param");
  m_viewParamBtn->signal_value_changed().connect(sigc::mem_fun(*this, &AlignmentView::queue_draw));

  m_aspectFrame = dynamic_cast<Gtk::AspectFrame*>(get_parent());

  m_refAspect = 0;

  viewTypeChanged();
}

void AlignmentView::connectState(const std::shared_ptr<UI::State>& state) {
  m_state = state;

  // Reload textures
  referenceChanged();
  sequenceViewSelectionChanged(0, 0);
}

void AlignmentView::realize() {
  GLAreaPlus::realize();

  m_program = wrap(GL::Program::from_path(get_context(), "ui/gl/align/vert.glsl", "ui/gl/align/frag.glsl"));

  m_refTexture = createTexture();
  m_alignTexture = createTexture();

  // Reload textures
  referenceChanged();
  sequenceViewSelectionChanged(0, 0);
}

bool AlignmentView::render(const Glib::RefPtr<Gdk::GLContext>& context) {
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  m_program->use();

  glActiveTexture(GL_TEXTURE0);
  m_refTexture->bind();
  glActiveTexture(GL_TEXTURE1);
  m_alignTexture->bind();

  m_program->uniform1i("u_RefTexture", 0);
  m_program->uniform1i("u_AlignTexture", 1);

  m_program->uniform4f("u_ViewSelection", m_viewSection);

  int viewType = static_cast<int>(m_viewTypeBtn->get_value());
  m_program->uniform1i("u_DisplayType", viewType);
  float viewParam = static_cast<float>(m_viewParamBtn->get_value());
  m_program->uniform1f("u_DisplayParam", viewParam);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  return true;
}

void AlignmentView::referenceChanged() {
  if(m_state == nullptr || !get_context())
    return;

  auto index = static_cast<int>(m_refImgBtn->get_value());

  auto& seqImg = m_state->m_sequence->image(index);
  auto params = m_state->m_imageFile.getImageParameters(seqImg.m_fileIndex);

  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  // Make large images use a subset of pixels to save VRAM
  // int scale = params.width() / 500;
  // if(scale < 0) scale = 1;
  // params.setDimension(0, -1, -1, scale);
  // params.setDimension(1, -1, -1, scale);

  if(params.type() != Img::DataType::SHORT && params.type() != Img::DataType::USHORT) {
    spdlog::error("Not a short type 16 bit image");
    return;
  }

  auto data = m_state->m_imageFile.getPixels(params);
  m_refTexture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);
  m_refAspect = (float)params.width() / params.height();

  queue_draw();
}

void AlignmentView::viewTypeChanged() {
  int viewType = m_viewTypeBtn->get_value();

  auto paramAdj = m_viewParamBtn->get_adjustment();
  switch(viewType) {
    case 0:
      paramAdj->set_lower(0);
      paramAdj->set_upper(1);
      paramAdj->set_step_increment(0.05);
      paramAdj->set_value(0.5);
      break;
    case 1:
    case 2:
      paramAdj->set_lower(0);
      paramAdj->set_upper(10);
      paramAdj->set_step_increment(0.1);
      paramAdj->set_value(5);
      break;
  }

  queue_draw();
}

void AlignmentView::sequenceViewSelectionChanged(uint position, uint nitems) {
  if(m_state == nullptr || !get_context())
    return;

  uint selectedIndex = m_sequenceView->getSelected();

  auto& seqImg = m_state->m_sequence->image(selectedIndex);
  auto params = m_state->m_imageFile.getImageParameters(seqImg.m_fileIndex);

  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  // Make large images use a subset of pixels to save VRAM
  // int scale = params.width() / 500;
  // if(scale < 0) scale = 1;
  // params.setDimension(0, -1, -1, scale);
  // params.setDimension(1, -1, -1, scale);

  if(params.type() != Img::DataType::SHORT && params.type() != Img::DataType::USHORT) {
    spdlog::error("Not a short type 16 bit image");
    return;
  }

  auto data = m_state->m_imageFile.getPixels(params);
  m_alignTexture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);

  queue_draw();
}

void AlignmentView::pickArea() {
  m_mainView->requestSelection([this](float x, float y, float width, float height) {
    m_viewSection[0] = x;
    m_viewSection[1] = y * m_refAspect;
    m_viewSection[2] = width;
    m_viewSection[3] = height * m_refAspect;

    m_aspectFrame->set_ratio(width / height);

    spdlog::info("Selected ({}, {}), ({}, {})", x, y, width, height);
    queue_draw();
  });
}

