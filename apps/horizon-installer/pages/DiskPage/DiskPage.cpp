#include "DiskPage.hpp"
#include <horizon-disk-utilities/DiskManager.hpp>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/ToolbarButton.hpp>

namespace horizon::installer
{
    class DiskIconItem : public Widget
    {
    public:
        DiskIconItem() : Widget()
        {
            auto icon = std::make_unique<Icon>();
            icon->set_position_type(FREE);
            icon->set_enabled(false);
            m_icon_ptr = icon.get();
            add_child(std::move(icon));

            auto label = std::make_unique<Label>();
            label->set_position_type(FREE);
            label->set_alignment(TextAlignment::Center);
            label->set_vertical_alignment(VerticalAlignment::Middle);
            label->set_enabled(false);
            m_label_ptr = label.get();
            add_child(std::move(label));

            m_position_type = FREE;
        }

        void set_data(const DiskData &d, float zoom, bool selected)
        {
            m_zoom = zoom;
            m_selected = selected;
            m_label_ptr->set_text(d.name + "\n(" + d.capacity + ")");

            std::string icon_name = d.is_ssd ? "drive-harddisk-solidstate" : "drive-harddisk";
            m_icon_size = static_cast<int>(64 * m_zoom);
            m_icon_ptr->set_icon_name(icon_name);
            m_icon_ptr->set_icon_size(m_icon_size);

            m_label_ptr->set_font_size(11 * m_zoom);

            invalidate();
        }

        int preferred_height(int width) const override
        {
            int padding = static_cast<int>(8 * m_zoom);
            int gap = 4;
            int label_h = m_label_ptr->preferred_height(width - 8);
            return padding + m_icon_size + gap + label_h + padding;
        }

        void calculate_layout() override
        {
            int padding = static_cast<int>(8 * m_zoom);
            int icon_y = padding;

            m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
            m_icon_ptr->set_size(m_icon_size, m_icon_size);

            int label_y = icon_y + m_icon_size + 4;
            m_label_ptr->set_position(m_x + 4, m_y + label_y);
            m_label_ptr->set_size(m_width - 8, m_height - label_y - 4);
        }

        void draw(GraphicsContext &gc) override
        {
            if (m_selected)
            {
                auto *tm = theme_manager();
                Color bg = tm->get_color("table_row_selected");
                Color hbg = bg;
                hbg.a = 0.3f; // Subtler selection

                gc.setColor(hbg);
                gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(8));

                gc.setColor(bg);
                gc.drawRect(m_x, m_y, m_width, m_height, CornerRadius(8), 2.0f);
            }
        }

    private:
        Icon *m_icon_ptr{nullptr};
        Label *m_label_ptr{nullptr};
        float m_zoom{1.0f};
        int m_icon_size{64};
        bool m_selected{false};
    };

    DiskPage::DiskPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto logo = std::make_unique<Icon>();
        logo->set_icon_name("system-os-installer");
        logo->set_icon_size(64);
        logo->set_size(64, 64);

        auto logo_container = std::make_unique<Widget>();
        logo_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        logo_container->add_child(Spacer());
        logo_container->add_child(std::move(logo));
        logo_container->add_child(Spacer());
        add_child(std::move(logo_container));

        auto title = std::make_unique<Label>(i18n().tr("installer.disk.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        title->set_fixed_size(50);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.disk.desc"));
        desc->set_alignment(TextAlignment::Center);
        desc->set_fixed_size(40);
        add_child(std::move(desc));

        add_child(Spacer(50));

        auto disk_tree = std::make_unique<IconView<DiskData>>();
        m_disk_tree = disk_tree.get();
        add_child(std::move(disk_tree));
        m_disk_tree->set_size(500, 300);
        m_disk_tree->set_item_size(120, 140);

        m_disk_tree->set_item_factory(
            [](const DiskData &data, float zoom, bool selected)
            {
                auto item = std::make_unique<DiskIconItem>();
                item->set_data(data, zoom, selected);
                return item;
            });

        refresh_disks();

        add_child(Spacer());

        auto btn_back =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.back"), "go-previous");
        btn_back->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_back.run(ctx);
            });

        auto btn_install =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.install"), "system-run");
        btn_install->when_click.connect(
            [this](auto &)
            {
                auto selected = m_disk_tree->get_selected_items();
                if (!selected.empty())
                {
                    selected_device_str = selected[0].name + " (" + selected[0].capacity + ")";
                    EventContext ctx;
                    when_install.run(ctx);
                }
            });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_back));
        btn_container->add_child(Spacer(20));
        btn_container->add_child(std::move(btn_install));
        btn_container->add_child(Spacer());
        btn_container->set_fixed_size(70);
        add_child(std::move(btn_container));
    }

    void DiskPage::refresh_disks()
    {
        horizon::disks::DiskManager manager;
        manager.scan();

        std::vector<DiskData> data;
        for (const auto &dev : manager.devices())
        {
            DiskData d;
            d.name = dev->name;
            d.path = dev->device_path;
            d.capacity = dev->human_capacity();
            d.is_ssd = dev->is_ssd;
            data.push_back(d);
        }
        m_disk_tree->set_data(std::move(data));
    }
} // namespace horizon::installer
