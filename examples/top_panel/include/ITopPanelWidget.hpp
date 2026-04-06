#pragma once

#include <horizon/Widget.hpp>
#include <string>

/**
 * @brief Interface for widgets that can be added to the Top Panel's Indicators Container.
 */
class ITopPanelWidget : public horizon::Widget
{
public:
    ITopPanelWidget() : horizon::Widget() 
    {
        // Top panel widgets typically have a fixed height matching the panel.
        // Their width is usually variable based on content.
        set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
    }
    
    virtual ~ITopPanelWidget() = default;

    /**
     * @brief Unique identifier for the widget (e.g., "clock", "network").
     */
    virtual std::string widget_id() const = 0;

    /**
     * @brief Display name for the widget (e.g., "Date and Time").
     */
    virtual std::string widget_name() const = 0;
};
