#pragma once

#include <memory>

#include <gtkmm.h>

namespace UI {
class State;
class Window;
}

namespace UI::Pages {

class Page {
  const std::string m_name;

protected:
  std::shared_ptr<State> m_state;
  
  Page(const std::string& name);

public:
  const std::string& name();

  virtual ~Page() = default;

  virtual void connectState(const std::shared_ptr<State>& state);
  virtual Glib::RefPtr<Gio::ActionGroup> actionGroup();

protected:
  template<class T>
  static std::unique_ptr<T> create(Gtk::Stack *stack, Window& window, const std::string& path, const std::string& title) {
    auto builder = Gtk::Builder::create_from_file(path);
    std::unique_ptr<T> ptr(new T(builder, window));

    auto topObj = builder->get_widget<Gtk::Widget>("root");
    stack->add(dynamic_cast<Gtk::Widget&>(*topObj), ptr->name(), title);
    return ptr;
  }
};

}

