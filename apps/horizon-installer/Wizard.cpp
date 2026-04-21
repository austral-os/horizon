#include "Wizard.hpp"

namespace horizon::installer
{
    Wizard::Wizard()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
    }

    Wizard::~Wizard() = default;

    void Wizard::add_page(std::unique_ptr<Widget> page)
    {
        page->set_visible(m_pages.empty());
        Widget* ptr = page.get();
        m_pages.push_back(ptr);
        add_child(std::move(page));
    }

    void Wizard::next()
    {
        if (m_current_index + 1 < m_pages.size())
        {
            m_pages[m_current_index]->set_visible(false);
            m_current_index++;
            m_pages[m_current_index]->set_visible(true);
        }
    }

    void Wizard::back()
    {
        if (m_current_index > 0)
        {
            m_pages[m_current_index]->set_visible(false);
            m_current_index--;
            m_pages[m_current_index]->set_visible(true);
        }
    }

    void Wizard::show_page(size_t index)
    {
        if (index < m_pages.size())
        {
            m_pages[m_current_index]->set_visible(false);
            m_current_index = index;
            m_pages[m_current_index]->set_visible(true);
        }
    }

    void Wizard::update_visible_page()
    {
        for (size_t i = 0; i < m_pages.size(); ++i)
        {
            m_pages[i]->set_visible(i == m_current_index);
        }
    }
} // namespace horizon::installer
