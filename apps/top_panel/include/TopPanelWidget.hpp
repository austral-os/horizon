#pragma once

#include <horizon/Panel.hpp>
#include <memory>
#include <string>

class TopPanelApplication;
class TopPanelMenuBar;
class IndicatorsContainer;

class TopPanelWidget : public horizon::Panel
{
public:
    TopPanelWidget(TopPanelApplication* app);
    virtual ~TopPanelWidget() = default;

    void handle_message(const std::string& msg);
    void draw(horizon::GraphicsContext& gc) override;
    
    IndicatorsContainer* indicators() const { return m_indicators; }

private:
    TopPanelMenuBar* m_menubar;
    IndicatorsContainer* m_indicators;
};
