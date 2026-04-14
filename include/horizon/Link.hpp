#pragma once

#include <horizon/Label.hpp>
#include <string>

namespace horizon
{
    /**
     * @brief A widget that displays a clickable link.
     * Inherits from Label and automatically opens the URL when clicked.
     */
    class Link : public Label
    {
    public:
        Link();
        explicit Link(const std::string &text, const std::string &url = "");
        ~Link() override = default;

        void set_url(const std::string &url);
        const std::string &url() const;

    private:
        std::string m_url;
    };

} // namespace horizon
