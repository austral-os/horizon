#pragma once
#include <horizon/MenuItem.hpp>
#include <horizon/MenuSeparator.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{

    class Menu : public Widget
    {
    public:
        Menu();
        ~Menu() = default;

        void add_item(std::unique_ptr<MenuItem> item);
        void add_separator();

        void render(GraphicsContext &ctx, int cx, int cy, int cw, int ch,
                    bool force = false) override;

        // Helper to add a text item directly
        MenuItem *add_item(const std::string &text, const std::string &shortcut = "");

        void calculate_layout() override;

        void close_submenus();
        void set_active_submenu(Menu *menu);

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        Menu *m_active_submenu = nullptr;
        int m_item_height = 24;
        int m_min_width = 200;
        bool m_was_visible = false; // Track visibility transitions for clearing
    };

} // namespace horizon
