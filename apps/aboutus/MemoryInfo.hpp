#pragma once

#include "horizon/CategorizedBar.hpp"
#include "horizon/FormatUtils.hpp"
#include "horizon/Label.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/SystemInfo.hpp"
#include "horizon/Widget.hpp"
#include <memory>
#include <vector>

namespace horizon
{
    class MemorySlotWidget : public Widget
    {
    public:
        MemorySlotWidget(const MemorySlotInfo &slot)
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            set_fixed_size(240); // Height will be derived or set separately if needed, but
                                 // Widget::set_fixed_size(int) exists
            set_margin(5);

            if (slot.occupied)
            {
                // Active slot style: Solid background with text
                auto box = std::make_unique<Widget>();
                box->set_background_color(Color("#6E6E6E")); // Grayish
                box->set_border_radius(3);
                box->set_border_width(1);
                box->set_border_color(Color(0.4f, 0.4f, 0.4f));
                box->set_layout_type(WIDGET_LAYOUT_VERTICAL);

                box->add_child(Spacer());
                auto label_row = std::make_unique<Widget>();
                label_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                label_row->add_child(Spacer(15)); // Left padding
                auto label = std::make_unique<Label>(format_bytes(slot.capacity));
                label->set_font_size(18);
                label->set_font_weight(FONT_WEIGHT_BOLD);
                label->set_text_color(Color(1.0f, 1.0f, 1.0f));
                label_row->add_child(std::move(label));
                box->add_child(std::move(label_row));
                box->add_child(Spacer());

                add_child(std::move(box));
            }
            else
            {
                // Empty slot style: Dashed border (simulated with light gray/border)
                auto box = std::make_unique<Widget>();
                box->set_background_color(Color(0.95f, 0.95f, 0.95f));
                box->set_border_radius(3);
                box->set_border_width(1);
                box->set_border_color(Color(0.7f, 0.7f, 0.7f));
                box->set_layout_type(WIDGET_LAYOUT_VERTICAL);

                box->add_child(Spacer());
                auto label_row = std::make_unique<Widget>();
                label_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                label_row->add_child(Spacer(15)); // Left padding
                auto label = std::make_unique<Label>("Empty");
                label->set_font_size(18);
                label->set_text_color(Color(0.6f, 0.6f, 0.6f));
                label_row->add_child(std::move(label));
                box->add_child(std::move(label_row));
                box->add_child(Spacer());

                add_child(std::move(box));
            }
        }
    };

    class MemoryInfoWidget : public Widget
    {
    public:
        MemoryInfoWidget()
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            set_margin(40);

            auto mem_info = SystemInfo::get_memory_info();

            // Header Section
            auto header = std::make_unique<Widget>();
            header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            header->set_fixed_size(120);

            // Large Icon/Badge on the left
            auto badge = std::make_unique<Widget>();
            badge->set_fixed_size(140);
            badge->set_background_color(Color("#2C6EB1")); // Blue
            badge->set_border_radius(3);
            badge->set_border_width(1);
            badge->set_border_color(Color("#1A4E8A"));
            badge->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            badge->set_margin(10);

            badge->add_child(Spacer());
            auto badge_value = std::make_unique<Label>(format_bytes(mem_info.total_capacity));
            badge_value->set_font_size(22);
            badge_value->set_font_weight(FONT_WEIGHT_BOLD);
            badge_value->set_text_color(Color(1, 1, 1));
            badge_value->set_fixed_size(30);

            auto badge_label = std::make_unique<Label>("Installed");
            badge_label->set_font_size(12);
            badge_label->set_text_color(Color(0.9f, 0.9f, 0.9f));
            badge_label->set_fixed_size(20);

            badge->add_child(std::move(badge_value));
            badge->add_child(std::move(badge_label));
            badge->add_child(Spacer());

            // Description text on the right
            auto description = std::make_unique<Widget>();
            description->set_layout_type(WIDGET_LAYOUT_VERTICAL);

            int occupied_count = 0;
            std::string type = "DDR3";
            uint32_t speed = 0;
            for (const auto &s : mem_info.slots)
            {
                if (s.occupied)
                {
                    occupied_count++;
                    type = s.type;
                    speed = s.speed;
                }
            }

            auto line1 = std::make_unique<Label>("Your Machine contains " +
                                                 std::to_string(mem_info.total_slots) +
                                                 " memory slots, each of which accepts");
            line1->set_fixed_size(25);
            auto line2 = std::make_unique<Label>("a " + std::to_string(speed) + " MHz " + type +
                                                 " memory module.");
            line2->set_fixed_size(25);
            auto line3 = std::make_unique<Label>(
                std::to_string(occupied_count) + " memory slots in use, " +
                std::to_string(mem_info.total_slots - occupied_count) + " available");
            line3->set_fixed_size(25);

            description->add_child(Spacer());
            description->add_child(std::move(line1));
            description->add_child(std::move(line2));
            description->add_child(Spacer(10));
            description->add_child(std::move(line3));
            description->add_child(Spacer());

            header->add_child(std::move(badge));
            header->add_child(Spacer(30));
            header->add_child(std::move(description));

            add_child(std::move(header));
            add_child(Spacer(20));

            // Slots Grid Container
            auto grid_bg = std::make_unique<Widget>();
            grid_bg->set_background_color(Color(0.92f, 0.92f, 0.92f));
            grid_bg->set_margin(20);
            grid_bg->set_border_radius(3);
            grid_bg->set_border_width(1);
            grid_bg->set_border_color(Color(0.8f, 0.8f, 0.8f));
            grid_bg->set_layout_type(WIDGET_LAYOUT_VERTICAL);

            auto row1 = std::make_unique<Widget>();
            row1->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row1->set_fixed_size(110);

            auto row2 = std::make_unique<Widget>();
            row2->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row2->set_fixed_size(110);

            for (size_t i = 0; i < mem_info.slots.size(); ++i)
            {
                if (i < 2)
                    row1->add_child(std::make_unique<MemorySlotWidget>(mem_info.slots[i]));
                else
                    row2->add_child(std::make_unique<MemorySlotWidget>(mem_info.slots[i]));

                if (i == 0 || i == 2)
                {
                    if (i < 2 && mem_info.slots.size() > 1)
                        row1->add_child(Spacer(20));
                    else if (i >= 2 && mem_info.slots.size() > 3)
                        row2->add_child(Spacer(20));
                }
            }

            int grid_height = 0;
            if (mem_info.slots.size() > 0)
            {
                grid_bg->add_child(std::move(row1));
                grid_height += 110;
            }
            if (mem_info.slots.size() > 2)
            {
                grid_bg->add_child(Spacer(10));
                grid_bg->add_child(std::move(row2));
                grid_height += 120; // 110 + 10 spacer
            }

            grid_bg->set_fixed_size(grid_height + 40); // + padding/margins

            add_child(std::move(grid_bg));
            add_child(Spacer());
        }

        virtual ~MemoryInfoWidget() = default;
    };
} // namespace horizon
