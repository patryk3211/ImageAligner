#pragma once

#include <gtkmm.h>

#include "ui/pages/page.hpp"
#include "cv/context.hpp"
#include "ui/widgets/alignment_view.hpp"

namespace UI::Pages {

class CV : public Page {
  Glib::RefPtr<Gio::SimpleActionGroup> m_actionGroup;

  SequenceView *m_sequenceList;

  Gtk::SpinButton *m_threshold;
  Gtk::SpinButton *m_descriptorSize;
  Gtk::SpinButton *m_descriptorChannels;
  Gtk::SpinButton *m_octaves;
  Gtk::SpinButton *m_octaveLayers;

  Gtk::CheckButton *m_onlySelected;

  std::shared_ptr<OpenCV::Context> m_cvContext;

private:
  CV(const Glib::RefPtr<Gtk::Builder>& builder, Window& window);

public:
  std::shared_ptr<OpenCV::Context> createCVContext(IO::ImageProvider& provider);

  virtual ~CV() = default;
  
  virtual Glib::RefPtr<Gio::ActionGroup> actionGroup() override;

  static std::unique_ptr<CV> load(Gtk::Stack *stack, Window& window) { return Page::create<CV>(stack, window, "ui/cv.blp.ui", "Auto-alignment"); }
  friend class Page;

private:
  void findKeypoints(const Glib::VariantBase& variant);
};

}

