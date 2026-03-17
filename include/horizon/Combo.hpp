#pragma once
#include <horizon/AirObject.hpp>
#include <horizon/EventsManager.hpp>
#include <memory>
#include <string>
#include <vector>

namespace horizon
{
    class Menu;
    class Widget;
    class GraphicsContext;

    struct ComboItem
    {
        std::string id;
        std::string text;
        std::string icon_name; // Optional
    };

    struct ComboItemSelectedContext
    {
        Widget *sender;
        ComboItem item;
        bool stop_propagation = false;
    };

    class Combo : public AirObject
    {
    public:
        Combo();
        virtual ~Combo();

        void add_item(const std::string &id, const std::string &text, const std::string &icon_name = "");
        void clear_items();

        void set_selected_item_by_id(const std::string &id);
        const ComboItem* selected_item() const;

        EventsManager<ComboItemSelectedContext> when_item_selected;

        void draw(GraphicsContext &gc) override;
        void calculate_layout() override;

    protected:
        void on_click();

    private:
        std::vector<ComboItem> m_items;
        int m_selected_index = -1;
        std::unique_ptr<Menu> m_menu;
        
        void update_menu();
        void handle_selection(int index);
    };
} // namespace horizon
