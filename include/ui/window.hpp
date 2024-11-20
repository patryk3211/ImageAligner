#pragma once

#include <gtkmm.h>

#include "adwaita.h"
#include "ui/widgets/alignment_view.hpp"
#include "ui/widgets/main_view.hpp"
#include "ui/widgets/sequence_list.hpp"
#include "ui/pages/page.hpp"

namespace UI {

class Window : public Gtk::Window {
  std::shared_ptr<UI::State> m_state;

  AdwDialog *m_saveChangesDialog;

  std::list<std::unique_ptr<Pages::Page>> m_toolPages;

public:
  SequenceView* m_sequenceView;
  MainView* m_mainView;
  AlignmentView* m_alignmentView;

public:
  Window();
  virtual ~Window() = default;

  void setState(const std::shared_ptr<UI::State>& state);

private:
  void showCloseDialog();
  bool closeRequest();

  void saveChangesFinish(GAsyncResult *result);

  template<class T> void addPage(Gtk::Stack *stack) {
    auto ptr = T::load(stack, *this);

    auto actionGroup = ptr->actionGroup();
    if(actionGroup != nullptr)
      this->insert_action_group(ptr->name(), actionGroup);

    m_toolPages.push_back(std::move(ptr));
  }
};

} // namespace UI

