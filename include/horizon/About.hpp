#pragma once
#include <algorithm>
#include <string>
#include <vector>

#ifndef HORIZON_CORE_VERSION
#define HORIZON_CORE_VERSION "0.0.0"
#endif

namespace horizon
{
    struct SomeoneInfo
    {
        std::string name;
        std::string url;
        std::string email;
    };

    struct About
    {
        std::string title;
        std::string description;
        std::string web;
        std::string git;
        std::string version;
        std::string icon;
        std::vector<SomeoneInfo> authors;
        std::vector<SomeoneInfo> translators;
    };

    const About ABOUT_HORIZON = {
        "Horizon",
        "Horizon is a free and open-source desktop environment for Linux.",
        "https://australos.hdrdevs.com.ar",
        "https://github.com/austral-os/horizon",
        HORIZON_CORE_VERSION,
        "horizon-desktop",
        {{"Horacio Daniel Ros", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com"}},
        {{"Horacio", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com"}}};

    class AboutManager
    {
    public:
        AboutManager() : m_horizon_data(ABOUT_HORIZON) {}

        // --- Gestión de Información de la Aplicación (m_app_data) ---

        void set_app_title(const std::string &title)
        {
            m_app_data.title = title;
        }
        void set_app_description(const std::string &description)
        {
            m_app_data.description = description;
        }
        void set_app_web(const std::string &web)
        {
            m_app_data.web = web;
        }
        void set_app_git(const std::string &git)
        {
            m_app_data.git = git;
        }
        void set_app_version(const std::string &version)
        {
            m_app_data.version = version;
        }
        void set_app_icon(const std::string &icon)
        {
            m_app_data.icon = icon;
        }

        void clear_app_title()
        {
            m_app_data.title.clear();
        }
        void clear_app_description()
        {
            m_app_data.description.clear();
        }
        void clear_app_web()
        {
            m_app_data.web.clear();
        }
        void clear_app_git()
        {
            m_app_data.git.clear();
        }
        void clear_app_version()
        {
            m_app_data.version.clear();
        }
        void clear_app_icon()
        {
            m_app_data.icon.clear();
        }

        const std::string &app_title() const
        {
            return m_app_data.title;
        }
        const std::string &app_description() const
        {
            return m_app_data.description;
        }
        const std::string &app_web() const
        {
            return m_app_data.web;
        }
        const std::string &app_git() const
        {
            return m_app_data.git;
        }
        const std::string &app_version() const
        {
            return m_app_data.version;
        }
        const std::string &app_icon() const
        {
            return m_app_data.icon;
        }

        void add_app_author(const SomeoneInfo &author)
        {
            m_app_data.authors.push_back(author);
        }
        void add_app_author(const std::string &name, const std::string &url = "",
                            const std::string &email = "")
        {
            m_app_data.authors.push_back({name, url, email});
        }
        void remove_app_author(const std::string &name)
        {
            m_app_data.authors.erase(
                std::remove_if(m_app_data.authors.begin(), m_app_data.authors.end(),
                               [&](const SomeoneInfo &a) { return a.name == name; }),
                m_app_data.authors.end());
        }
        void clear_app_authors()
        {
            m_app_data.authors.clear();
        }
        const std::vector<SomeoneInfo> &app_authors() const
        {
            return m_app_data.authors;
        }

        void add_app_translator(const SomeoneInfo &translator)
        {
            m_app_data.translators.push_back(translator);
        }
        void add_app_translator(const std::string &name, const std::string &url = "",
                                const std::string &email = "")
        {
            m_app_data.translators.push_back({name, url, email});
        }
        void remove_app_translator(const std::string &name)
        {
            m_app_data.translators.erase(
                std::remove_if(m_app_data.translators.begin(), m_app_data.translators.end(),
                               [&](const SomeoneInfo &a) { return a.name == name; }),
                m_app_data.translators.end());
        }
        void clear_app_translators()
        {
            m_app_data.translators.clear();
        }
        const std::vector<SomeoneInfo> &app_translators() const
        {
            return m_app_data.translators;
        }

        // --- Acceso a Información de Horizon (m_horizon_data) ---

        const About &horizon_data() const
        {
            return m_horizon_data;
        }
        const About &app_data() const
        {
            return m_app_data;
        }

    private:
        About m_horizon_data;
        About m_app_data;
    };
} // namespace horizon