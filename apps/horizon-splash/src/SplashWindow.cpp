#include "SplashWindow.hpp"
#include "horizon/Icon.hpp"
#include <horizon/Logger.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/I18n.hpp>

namespace horizon
{
    SplashWindow::SplashWindow() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);

        // Centering
        add_child(Spacer());

        // Logo
        auto image_container = std::make_unique<Widget>();
        image_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        image_container->add_child(Spacer());

        auto logo = std::make_unique<Icon>();
        logo->set_icon_name("emblem-austral");
        logo->set_icon_size(198);
        logo->set_fixed_size(198);
        image_container->add_child(std::move(logo));
        image_container->add_child(Spacer());
        image_container->set_fixed_size(198);

        add_child(std::move(image_container));

        add_child(Spacer(35));

        // Progress bar container
        auto progress_container = std::make_unique<Widget>();
        progress_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        progress_container->set_fixed_size(10);
        progress_container->add_child(Spacer());

        auto progress_bar = std::make_unique<ProgressBar>();
        progress_bar->set_fixed_size(400);
        progress_bar->set_progress(0.0f);
        m_progress_bar = progress_bar.get();
        progress_container->add_child(std::move(progress_bar));
        progress_container->add_child(Spacer());

        add_child(std::move(progress_container));

        add_child(Spacer(10));

        auto label = std::make_unique<Label>(i18n().tr("horizon_splash.loading"));
        label->set_font_size(16);
        label->set_fixed_size(35);
        label->set_text_color(Color(1.0f, 1.0f, 1.0f));
        label->set_alignment(TextAlignment::Center);
        // Ensure the label is centered within its own bounding box as well
        label->set_alignment(TextAlignment::Center);
        m_label = label.get();

        add_child(std::move(label));

        // Bottom centering
        add_child(Spacer());
    }

    void SplashWindow::draw(GraphicsContext &gc)
    {
        // Opaque black background
        gc.setColor(Color(0.0f, 0.0f, 0.0f, 1.0f));
        gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(0));
    }

    void SplashWindow::update_status(const std::string &text, int progress)
    {
        if (m_label && !text.empty())
        {
            m_label->set_text(text);
        }
        if (m_progress_bar)
        {
            m_progress_bar->set_progress(progress / 100.0f);
        }
    }
} // namespace horizon
