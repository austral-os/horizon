#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Button.hpp>
#include <horizon/EventsManager.hpp>
#include <vector>
#include <string>
#include <memory>

namespace horizon {

class TabCollection : public Widget {
public:
    TabCollection();
    virtual ~TabCollection() = default;

    /**
     * @brief Structure to hold data for a single tab.
     */
    struct TabPage {
        std::string title;
        Widget* body;
    };

    /**
     * @brief Adds a new tab to the collection.
     * @param title The title of the tab.
     * @param body The widget to be displayed as the tab's content.
     * @return The index of the newly added tab.
     */
    int add_tab(const std::string& title, std::unique_ptr<Widget> body);

    /**
     * @brief Removes a tab by its index.
     * @param index The index of the tab to remove.
     */
    void remove_tab(int index);

    /**
     * @brief Sets the currently active tab.
     * @param index The index of the tab to activate.
     */
    void set_current_tab(int index);

    /**
     * @return The index of the currently active tab.
     */
    int current_tab_index() const { return m_current_tab; }

    /**
     * @return Pointer to the body of the currently active tab.
     */
    Widget* current_tab_body() const;

    /**
     * @brief Updates the title of an existing tab.
     * @param index The index of the tab.
     * @param title The new title.
     */
    void set_tab_title(int index, const std::string& title);
    
    /**
     * @brief Shows or hides the tab header (the bar with the tabs and + button).
     */
    void show_header(bool visible);

    /**
     * @brief Sets whether the header should be automatically hidden when there is only one tab.
     * @param enabled True to enable smart header behavior, false otherwise.
     */
    void set_smart_header(bool enabled);

    /**
     * @return True if smart header behavior is enabled.
     */
    bool smart_header() const { return m_smart_header; }

    /**
     * @brief Sets whether tabs should have a close button.
     * @param enabled True to enable close buttons, false otherwise.
     */
    void set_closable_tabs(bool enabled);

    /**
     * @return True if closable tabs are enabled.
     */
    bool closable_tabs() const { return m_closable_tabs; }

    /**
     * @return The number of tabs in the collection.
     */
    size_t tab_count() const { return m_tabs.size(); }

    // --- Signals ---
    EventsManager<int> when_tab_added;      /**< Emitted when a tab is added (index). */
    EventsManager<int> when_items_changed;  /**< Emitted when the tab list changes (count). */
    EventsManager<int> when_tab_selected;   /**< Emitted when a tab is selected (index). */
    EventsManager<int> when_tab_close_requested; /**< Emitted when a tab's close button is clicked. */
    EventsManager<EventContext> when_add_tab_clicked; /**< Emitted when the "+" button is clicked. */

protected:
    void render(GraphicsContext& ctx, int cx, int cy, int cw, int ch, bool force = false) override;
    void draw(GraphicsContext& ctx) override;

private:
    void update_layout();

    /**
     * @brief Internal widget for rendering a single tab button in the header.
     */
    class TabButton : public Widget {
        std::string m_title;
        bool m_active = false;
        TabCollection* m_owner;
        int m_index;
        Button<SolidObject>* m_close_button = nullptr;

    public:
        TabButton(TabCollection* owner, int index, const std::string& title);
        void set_active(bool active);
        void set_title(const std::string& title);
        void set_index(int index) { m_index = index; }
        void draw(GraphicsContext& ctx) override;
        int preferred_width() const override;
    };

    Widget* m_header;    /**< Header containing the tab buttons. */
    Widget* m_container; /**< Container showing the active tab's body. */
    Widget* m_add_button;/**< The "+" button for adding tabs. */
    
    std::vector<TabPage> m_tabs;
    int m_current_tab = -1;
    bool m_smart_header = false;
    bool m_closable_tabs = false;
};

} // namespace horizon
