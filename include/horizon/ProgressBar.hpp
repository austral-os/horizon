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
        ~ProgressBar();

        void draw(GraphicsContext &gc) override;
        void set_application_recursive(Application *app) override;

        /**
         * @brief Set the current progress (0.0 to 1.0).
         */
        void set_progress(float progress);

        /**
         * @brief Get the current progress.
         */
        float progress() const;

        /**
         * @brief Set whether the progress bar is in an indeterminate state.
         */
        void set_indeterminate(bool indeterminate);

        /**
         * @return True if the progress bar is in an indeterminate state.
         */
        bool is_indeterminate() const;

    private:
        float m_progress{0.0f};
        float m_target_progress{0.0f};
        bool m_is_indeterminate{false};
        float m_animation_offset{0.0f};
        size_t m_timer_id{0};
    };
} // namespace horizon
