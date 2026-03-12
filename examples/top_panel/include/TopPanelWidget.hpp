#pragma once

#include <horizon/Panel.hpp>
#include <memory>
#include <string>

class TopPanelApplication;
class TopPanelMenuBar;

class TopPanelWidget : public horizon::Panel
{
public:
    TopPanelWidget(TopPanelApplication* app);
    virtual ~TopPanelWidget() = default;

    void handle_message(const std::string& msg);

private:
    TopPanelMenuBar* m_menubar;
};
