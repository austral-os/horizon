#pragma once

#include <horizon/Icon.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace horizon
{

    class DockApplication;

    /**
     * @brief Base class for special dock applets (like Trash, Downloads)
     */
    class DockApplet : public Icon
    {
    public:
        DockApplet(DockApplication *app, const std::string &name, const std::string &icon_name);
        virtual ~DockApplet() = default;

        virtual void load_config(const nlohmann::json &config) {}
        void draw(GraphicsContext& ctx) override;

        const std::string &name() const { return m_name; }

    protected:
        DockApplication *m_app;
        std::string m_name;
    };

    /**
     * @brief Trash Can applet
     */
    class TrashApplet : public DockApplet
    {
    public:
        TrashApplet(DockApplication *app);
        ~TrashApplet() override;

    private:
        void update_icon();
        size_t m_timer_id{0};
    };



} // namespace horizon
