#include "horizon/download/DownloadPopover.hpp"
#include "horizon/download/DownloadItemWidget.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/Button.hpp"
#include "horizon/Application.hpp"
#include "horizon/Window.hpp"
#include "horizon/SolidObject.hpp"
#include "horizon/Icon.hpp"

namespace horizon {
namespace download {

DownloadPopover::DownloadPopover(horizon::Widget* parent)
    : Widget() {
    
    set_size(350, 400);
    set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));

    auto main_layout = std::make_unique<horizon::Widget>();
    main_layout->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);

    // Header
    auto header = std::make_unique<horizon::Widget>();
    header->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);
    header->set_margin(10);
    
    auto title = std::make_unique<horizon::Label>("Descargas");
    title->set_font_weight(FONT_WEIGHT_BOLD);
    header->add_child(std::move(title));
    header->add_child(horizon::Spacer());

    auto clear_btn = std::make_unique<horizon::Button<horizon::SolidObject>>();
    clear_btn->set_size(30, 30);
    auto c_icon = std::make_unique<horizon::Icon>();
    c_icon->set_icon_name("edit-clear-symbolic");
    c_icon->set_icon_size(16);
    clear_btn->add_child(std::move(c_icon));
    clear_btn->when_click.connect([this](horizon::MouseButtonEventContext&) {
        this->refresh();
    });
    header->add_child(std::move(clear_btn));

    main_layout->add_child(std::move(header));

    // Scroll Area
    auto scroll = std::make_unique<horizon::ScrollArea>();
    auto container = std::make_unique<horizon::Widget>();
    container->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
    m_container = container.get();
    
    scroll->set_content(std::move(container));
    main_layout->add_child(std::move(scroll));

    add_child(std::move(main_layout));

    refresh();
}

void DownloadPopover::show_relative_to(horizon::Widget* target) {
    if (!target) return;
    
    // Position it below the target
    int px = target->x() + target->width() - width();
    int py = target->y() + target->height() + 5;
    
    set_position(px, py);
}

void DownloadPopover::draw(GraphicsContext& gc) {
    // Shadow/Border
    gc.setColor(Color(0.0f, 0.0f, 0.0f, 0.2f));
    gc.fillRect(x() + 2, y() + 2, width(), height(), CornerRadius(8));
    
    gc.setColor(Color(0.8f, 0.8f, 0.8f, 1.0f));
    gc.fillRect(x(), y(), width(), height(), CornerRadius(8));
    
    gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    gc.fillRect(x() + 1, y() + 1, width() - 2, height() - 2, CornerRadius(7));
    
    Widget::draw(gc);
}

void DownloadPopover::refresh() {
    m_container->clear_children();
    
    auto tasks = DownloadManager::instance().tasks();
    
    if (tasks.empty()) {
        auto empty = std::make_unique<horizon::Label>("No hay descargas activas");
        empty->set_margin(20);
        m_container->add_child(std::move(empty));
    } else {
        for (auto& task : tasks) {
            m_container->add_child(std::make_unique<DownloadItemWidget>(task));
        }
    }
}

} // namespace download
} // namespace horizon
