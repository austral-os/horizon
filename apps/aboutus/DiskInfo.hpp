#pragma once

#include "horizon/CategorizedBar.hpp"
#include "horizon/FormatUtils.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/SystemInfo.hpp"
#include "horizon/Widget.hpp"
#include <memory>

namespace horizon
{
    class DiskInfoWidget : public Widget
    {
    public:
        DiskInfoWidget()
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            set_margin(40);

            auto disk_info = SystemInfo::get_os_disk_info();

            // Header: Icon + Info
            auto header = std::make_unique<Widget>();
            header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            header->set_fixed_size(80);

            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("drive-harddisk");
            icon->set_icon_size(64);
            icon->set_fixed_size(80);

            auto text_info = std::make_unique<Widget>();
            text_info->set_layout_type(WIDGET_LAYOUT_VERTICAL);

            auto model_label = std::make_unique<Label>(disk_info.brand + " " + disk_info.model);
            model_label->set_font_size(24);
            model_label->set_font_weight(FONT_WEIGHT_BOLD);
            model_label->set_fixed_size(35);

            auto cap_label =
                std::make_unique<Label>("Total Capacity: " + format_bytes(disk_info.capacity));
            cap_label->set_font_size(16);
            cap_label->set_text_color(Color(0.4f, 0.4f, 0.4f));
            cap_label->set_fixed_size(25);

            text_info->add_child(std::move(model_label));
            text_info->add_child(std::move(cap_label));

            header->add_child(std::move(icon));
            header->add_child(Spacer(20));
            header->add_child(std::move(text_info));

            add_child(std::move(header));
            add_child(Spacer(40));

            // The Bar
            auto bar = std::make_unique<CategorizedBar>();
            bar->set_fixed_size(150); // Height for bar + legend
            bar->set_total_value(disk_info.capacity);
            bar->set_value_formatter(format_bytes);

            std::vector<Color> partition_colors = {
                Color("#007AFF"), // Blue
                Color("#34C759"), // Green
                Color("#FFCC00"), // Yellow
                Color("#FF3B30"), // Red
                Color("#AF52DE"), // Purple
                Color("#5856D6")  // Indigo
            };

            uint64_t total_assigned = 0;
            for (size_t i = 0; i < disk_info.partitions.size(); ++i)
            {
                const auto &p = disk_info.partitions[i];
                std::string label = (p.mount_point == "/") ? "System (Root)" : p.mount_point;
                bar->add_category(label, p.capacity, partition_colors[i % partition_colors.size()]);
                total_assigned += p.capacity;
            }

            // Unused space
            if (disk_info.capacity > total_assigned)
            {
                bar->add_category("Other / Unallocated", disk_info.capacity - total_assigned,
                                  Color("#8E8E93"));
            }

            add_child(std::move(bar));
            add_child(Spacer());
        }

        virtual ~DiskInfoWidget() = default;
    };
} // namespace horizon
