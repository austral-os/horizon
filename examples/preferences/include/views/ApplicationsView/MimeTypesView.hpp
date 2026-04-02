#pragma once
#include <horizon/Widget.hpp>
#include <horizon/TreeView.hpp>
#include <horizon/Label.hpp>
#include <horizon/SearchBox.hpp>
#include <string>
#include <map>
#include <vector>

namespace horizon::preferences
{
    class MimeTypesView : public Widget
    {
    public:
        MimeTypesView();
        ~MimeTypesView() override = default;

        void load_mime_types();
        void update_tree(const std::string& filter = "");

    private:
        SearchBox* m_search_box{nullptr};
        TreeView* m_tree_view{nullptr};
        Label* m_selected_mime_label{nullptr};
        std::map<std::string, std::vector<std::string>> m_mime_data;
    };
} // namespace horizon::preferences
