#include "ui/pages/page.hpp"

using namespace UI::Pages;

Page::Page(const std::string& name)
  : m_name(name) {

}

const std::string& Page::name() {
  return m_name;
}

void Page::connectState(const std::shared_ptr<State>& state) {
  m_state = state;
}

Glib::RefPtr<Gio::ActionGroup> Page::actionGroup() {
  return nullptr;
}

