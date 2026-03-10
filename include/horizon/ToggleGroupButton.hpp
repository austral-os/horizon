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

        void add_item(std::string text) override;
        void add_item(std::unique_ptr<Icon> icon) override;

    protected:
        void configure() override;

    private:
        int m_current_index{-1};
    };
} // namespace horizon
