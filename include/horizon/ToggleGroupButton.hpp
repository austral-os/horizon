#pragma once

#include <horizon/GroupButton.hpp>

namespace horizon
{
    class ToggleGroupButton : public GroupButton
    {
    public:
        ToggleGroupButton();
        ~ToggleGroupButton();

        void set_current_item(int index);
        int current_item() const;

        void add_item(std::string text, int width = -1) override;
        void add_item(std::unique_ptr<Icon> icon, int width = -1) override;

    protected:
        void configure() override;

    private:
        int m_current_index{-1};
    };
} // namespace horizon
