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

  m_xOffsetBtn = builder->get_widget<Gtk::SpinButton>("x_offset_spin_btn");
  m_yOffsetBtn = builder->get_widget<Gtk::SpinButton>("y_offset_spin_btn");

  m_aspectFrame = dynamic_cast<Gtk::AspectFrame*>(get_parent());

  m_imageRegistration = 0;
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
  spdlog::trace("Alignment view render start");

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  if(!m_referenceImage || !m_alignImage)
    return true;

  m_program->use();

  glActiveTexture(GL_TEXTURE0);
  m_refTexture->bind();
  glActiveTexture(GL_TEXTURE1);
  m_alignTexture->bind();

  m_program->uniform1i("u_RefTexture", 0);
  m_program->uniform1i("u_AlignTexture", 1);

  m_program->uniform4f("u_ViewSelection", m_viewSection);

  float matrix[9];
  if(!m_alignImage->isReference()) {
    m_alignImage->homography().read(matrix);

    // Scale translations
    matrix[2] *= -m_pixelSize;
    matrix[5] *=  m_pixelSize * m_refAspect;
  } else {
    // Identity matrix
    memset(matrix, 0, sizeof(matrix));

    matrix[0] = 1;
    matrix[4] = 1;
    matrix[8] = 1;
  }
  m_program->uniformMat3fv("u_Homography", 1, true, matrix);

  int viewType = static_cast<int>(m_viewTypeBtn->get_value());
  m_program->uniform1i("u_DisplayType", viewType);
  float viewParam = static_cast<float>(m_viewParamBtn->get_value());
  m_program->uniform1f("u_DisplayParam", viewParam);

  m_program->uniform2f("u_RefLevels", m_referenceImage->getScaledMinLevel(), m_referenceImage->getScaledMaxLevel());
  m_program->uniform2f("u_AlignLevels", m_alignImage->getScaledMinLevel(), m_alignImage->getScaledMaxLevel());

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  spdlog::trace("Alignment view render end");
  return true;
}

void AlignmentView::referenceChanged() {
  if(m_state == nullptr || !get_context())
    return;

  spdlog::trace("(AlignmentView) Reference changed");

  auto index = static_cast<int>(m_refImgBtn->get_value());
  m_referenceImage = m_sequenceView->getImage(index);

  auto params = m_referenceImage->getProvider().getImageParameters(m_referenceImage->getSequenceImage().m_fileIndex);

  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  m_pixelSize = 1.0 / params.width();
  m_refAspect = (float)params.width() / params.height();

  if(params.type() != Img::DataType::SHORT && params.type() != Img::DataType::USHORT) {
    spdlog::error("Not a short type 16 bit image");
    return;
  }

  auto data = m_state->m_imageFile.getPixels(params);
  m_refTexture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);

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

  spdlog::trace("(AlignmentView) Sequence view selection changed");

  if(m_xBinding) {
    m_xBinding->unbind();
    m_xBinding = nullptr;
  }

  if(m_yBinding) {
    m_yBinding->unbind();
    m_yBinding = nullptr;
  }

  m_alignImage = m_sequenceView->getSelected();
  auto params = m_alignImage->getProvider().getImageParameters(m_alignImage->getSequenceImage().m_fileIndex);
  if(!m_alignImage->isReference()) {
    m_xOffsetBtn->set_editable(true);
    m_yOffsetBtn->set_editable(true);

    m_xBinding = Glib::Binding::bind_property(m_alignImage->propertyXOffset(), m_xOffsetBtn->property_value(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
    m_yBinding = Glib::Binding::bind_property(m_alignImage->propertyYOffset(), m_yOffsetBtn->property_value(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
  } else {
    m_imageRegistration = 0;
    m_xOffsetBtn->set_value(0);
    m_yOffsetBtn->set_value(0);

    m_xOffsetBtn->set_editable(false);
    m_yOffsetBtn->set_editable(false);
  }

  if(!m_alignSigConn.empty())
    m_alignSigConn.disconnect();
  m_alignSigConn = m_alignImage->signalRedraw().connect(sigc::mem_fun(*this, &AlignmentView::queue_draw));

  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

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

