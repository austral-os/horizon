#include "IndicatorsContainer.hpp"
#include <horizon/Widget.hpp>
#include <numeric>
#include <algorithm>

using namespace horizon;

IndicatorsContainer::IndicatorsContainer() : Widget()
{
    set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    set_spacing(10);
    set_margin(5);
}

void IndicatorsContainer::add_indicator(std::unique_ptr<ITopPanelWidget> indicator)
{
    // Ensure indicators have variable width based on preferences
    // and fixed height matching the container.
    add_child(std::move(indicator));
    invalidate();
}

void IndicatorsContainer::remove_indicator(const std::string& id)
{
    // Need to find by ID
    for (int i = 0; i < (int)m_children.size(); ++i)
    {
        auto* widget = dynamic_cast<ITopPanelWidget*>(m_children[i].get());
        if (widget && widget->widget_id() == id)
        {
            remove_child_at(i);
            break;
        }
    }
    invalidate();
}

void IndicatorsContainer::move_indicator(int from_index, int to_index)
{
    if (from_index < 0 || from_index >= (int)m_children.size() || 
        to_index < 0 || to_index >= (int)m_children.size())
    {
        return;
    }
    
    std::unique_ptr<Widget> child = std::move(m_children[from_index]);
    m_children.erase(m_children.begin() + from_index);
    m_children.insert(m_children.begin() + to_index, std::move(child));
    invalidate();
}

int IndicatorsContainer::preferred_width() const
{
    int total_width = 0;
    int visible_children = 0;
    
    for (const auto& child : m_children)
    {
        if (child->is_visible())
        {
            total_width += child->preferred_width();
            visible_children++;
        }
    }
    
    if (visible_children > 1)
    {
        total_width += (visible_children - 1) * spacing();
    }
    
    return total_width + (margin() * 2);
}

void IndicatorsContainer::calculate_layout()
{
    // First, calculate each indicator's width and set it as fixed size 
    // to ensure the Widget-level horizontal layout works correctly.
    for (const auto& child : m_children)
    {
        if (child->is_visible())
        {
            // Indicator width is determined by its preferred width.
            child->set_fixed_size(child->preferred_width());
        }
    }
    
    // Set our own fixed size based on children's preferred widths
    // to ensure we don't take more space than needed in the main panel.
    set_fixed_size(preferred_width());
    
    Widget::calculate_layout();
}
