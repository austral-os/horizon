#pragma once

#include <horizon/Widget.hpp>

namespace horizon
{
    /**
     * @brief A progress bar widget with Aqua styling.
     */
    class ProgressBar : public Widget
    {
    public:
        ProgressBar();
        ~ProgressBar() = default;

        void draw(GraphicsContext &gc) override;

        /**
         * @brief Set the current progress (0.0 to 1.0).
         */
        void set_progress(float progress);

        /**
         * @brief Get the current progress.
         */
        float progress() const;

    private:
        float m_progress{0.0f};
    };
} // namespace horizon
