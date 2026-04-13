#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <string>
#include <memory>

namespace horizon
{
    class DialogPreferences : public WaylandWindow
    {
    public:
        DialogPreferences(const std::string &title, int width = 600, int height = 400);
        ~DialogPreferences() override = default;

        /**
         * @brief Returns the internal toolbar.
         */
        Toolbar *toolbar() const;

        /**
         * @brief Sets the content widget of the dialog.
         */
        void set_content(std::unique_ptr<Widget> content);

        /**
         * @brief Returns the content widget of the dialog.
         */
        Widget *content() const;

    private:
        ApplicationWindow *m_app_window{nullptr};
    };
} // namespace horizon
