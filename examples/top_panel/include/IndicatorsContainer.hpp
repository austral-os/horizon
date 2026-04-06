#pragma once

#include "ITopPanelWidget.hpp"
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

/**
 * @brief Container that holds and manages right-alignedIndicators in the top panel.
 */
class IndicatorsContainer : public horizon::Widget
{
public:
    IndicatorsContainer();
    virtual ~IndicatorsContainer() = default;

    /**
     * @brief Adds a new indicator widget to the container.
     */
    void add_indicator(std::unique_ptr<ITopPanelWidget> indicator);

    /**
     * @brief Removes an indicator widget by ID.
     */
    void remove_indicator(const std::string& id);

    /**
     * @brief Reorders indicators by index.
     */
    void move_indicator(int from_index, int to_index);

    /**
     * @brief Returns the layout's preferred width (sum of child widths).
     */
    int preferred_width() const override;

    void calculate_layout() override;
};
