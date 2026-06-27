#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Image.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Label.hpp>

namespace horizon
{
    class SplashWindow : public Widget
    {
    public:
        SplashWindow();
        ~SplashWindow() override = default;

        void draw(GraphicsContext &gc) override;
        
        void update_status(const std::string &text, int progress);

    private:
        ProgressBar *m_progress_bar = nullptr;
        Label *m_label = nullptr;
    };
} // namespace horizon
