#include "ui/widgets/alignment_view.hpp"
#include "objects/matrix.hpp"
#include "objects/registration.hpp"
#include "ui/state.hpp"

#include <GL/gl.h>
#include <spdlog/spdlog.h>

using namespace UI;
using namespace Obj;
using namespace IO;

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

  // m_viewSection[0] = 0;
  // m_viewSection[1] = 0;
  // m_viewSection[2] = 1;
  // m_viewSection[3] = 1;

  // m_imageRegistration = 0;
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
  spdlog::trace("(AlignmentView) Render start");

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  if(!m_referenceImage || !m_alignImage) {
    spdlog::trace("(AlignmentView) Render quick end");
    return true;
  }

  m_program->use();

  glActiveTexture(GL_TEXTURE0);
  m_refTexture->bind();
  glActiveTexture(GL_TEXTURE1);
  m_alignTexture->bind();

  m_program->uniform1i("u_RefTexture", 0);
  m_program->uniform1i("u_AlignTexture", 1);

  if(m_viewSection) {
    m_program->uniform4f("u_ViewSelection",
                         m_viewSection->m_x,
                         m_viewSection->m_y * m_refAspect,
                         m_viewSection->m_width,
                         m_viewSection->m_height * m_refAspect);
  } else {
    m_program->uniform4f("u_ViewSelection", 0, 0, 1, 1);
  }

  float matrix[9];
  if(!m_alignImage->isReference()) {
    m_alignImage->getRegistration()->matrix().read(matrix);;

    // Scale translations
    matrix[2] *= -m_pixelSize;
    matrix[5] *=  m_pixelSize * m_refAspect;
  } else {
    // Identity matrix
    HomographyMatrix::identity(matrix);
  }
  m_program->uniformMat3fv("u_Homography", 1, true, matrix);

  int viewType = static_cast<int>(m_viewTypeBtn->get_value());
  m_program->uniform1i("u_DisplayType", viewType);
  float viewParam = static_cast<float>(m_viewParamBtn->get_value());
  m_program->uniform1f("u_DisplayParam", viewParam);

  auto refStats = m_referenceImage->getStats(0);
  auto aliStats = m_alignImage->getStats(0);
  double typeMax = m_state->m_imageFile.maxTypeValue();
  m_program->uniform2f("u_RefLevels", refStats->getMin() / typeMax, refStats->getMax() / typeMax);
  m_program->uniform2f("u_AlignLevels", aliStats->getMin() / typeMax, aliStats->getMax() / typeMax);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  spdlog::trace("(AlignmentView) Render end");
  return true;
}

void AlignmentView::referenceChanged() {
  if(m_state == nullptr || !get_context())
    return;

  spdlog::trace("(AlignmentView) Reference changed");

  auto index = static_cast<int>(m_refImgBtn->get_value());
  m_referenceImage = m_sequenceView->getImage(index);

  if(!m_referenceImage->getStats(0)) {
    // Guarantee that stats are available for drawing
    m_referenceImage->calculateStats(m_state->m_imageFile);
  }

  auto params = m_state->m_imageFile.getImageParameters(m_referenceImage->getFileIndex());

  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  m_pixelSize = 1.0 / params.width();
  m_refAspect = (float)params.width() / params.height();

  if(params.type() != DataType::SHORT && params.type() != DataType::USHORT) {
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

  // Remove old bindings
  if(m_xBinding) {
    m_xBinding->unbind();
    m_xBinding = nullptr;
  }
  if(m_yBinding) {
    m_yBinding->unbind();
    m_yBinding = nullptr;
  }
  if(!m_alignSigConn.empty())
    m_alignSigConn.disconnect();

  // Get new image
  m_alignImage = m_sequenceView->getSelected();

  if(!m_alignImage->getStats(0))
    m_alignImage->calculateStats(m_state->m_imageFile);
  if(!m_alignImage->getRegistration())
    m_alignImage->setRegistration(Registration::create());

  // Create new bindings to fields
  if(!m_alignImage->isReference()) {
    m_xOffsetBtn->set_editable(true);
    m_yOffsetBtn->set_editable(true);

    m_xBinding = Glib::Binding::bind_property(m_alignImage->propertyXOffset(), m_xOffsetBtn->property_value(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
    m_yBinding = Glib::Binding::bind_property(m_alignImage->propertyYOffset(), m_yOffsetBtn->property_value(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
  } else {
    m_xOffsetBtn->set_value(0);
    m_yOffsetBtn->set_value(0);

    m_xOffsetBtn->set_editable(false);
    m_yOffsetBtn->set_editable(false);
  }

  // New redraw signal
  m_alignSigConn = m_alignImage->signalRedraw().connect(sigc::mem_fun(*this, &AlignmentView::queue_draw));

  auto params = m_state->m_imageFile.getImageParameters(m_alignImage->getFileIndex());
  // Read only the first layer
  params.setDimension(2, 1, 1, 1);

  if(params.type() != DataType::SHORT && params.type() != DataType::USHORT) {
    spdlog::error("Not a short type 16 bit image");
    return;
  }

  auto data = m_state->m_imageFile.getPixels(params);
  m_alignTexture->load(params.width(), params.height(), GL_RED, GL_UNSIGNED_SHORT, data.get(), GL_R16);

  queue_draw();
}

void AlignmentView::pickArea() {
  m_mainView->requestSelection([this](const std::shared_ptr<Selection>& selection) {
    m_viewSection = selection;
    m_aspectFrame->set_ratio(selection->m_width / selection->m_height);

    spdlog::info("Selected ({}, {}), ({}, {})", selection->m_x, selection->m_y, selection->m_width, selection->m_height);
    queue_draw();
  });
}

