#pragma once

#include <horizon/Widget.hpp>
#include <horizon/TreeView.hpp>
#include <horizon/Label.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/TableView.hpp>
#include <string>
#include <map>
#include <vector>

namespace horizon::preferences
{
    struct ApplicationInfo
    {
        std::string name;
        std::string icon;
    };

    struct MimeExtension
    {
        std::string pattern;
        bool is_user;
    };

    class MimeTypesView : public Widget
    {
    public:
        MimeTypesView();
        ~MimeTypesView() override = default;

        void load_mime_types();
        void update_tree(const std::string& filter = "");
        void update_details(const std::string& mime_type);

    private:
        void setup_left_column();
        void setup_right_column();
        
        void on_add_extension();
        void on_remove_extension();
        void on_add_app();
        void on_remove_app();
        void on_move_app_up();
        void on_move_app_down();

    private:
        SearchBox* m_search_box{nullptr};
        TreeView* m_tree_view{nullptr};
        
        // Right side details
        Label* m_mime_title_label{nullptr};
        TableView<MimeExtension>* m_extensions_table{nullptr};
        TableView<ApplicationInfo>* m_apps_table{nullptr};

        std::string m_current_mime;
        std::map<std::string, std::vector<std::string>> m_mime_data;
        std::map<std::string, std::vector<MimeExtension>> m_mime_extensions;
        std::map<std::string, std::vector<ApplicationInfo>> m_mime_apps;
    };
} // namespace horizon::preferences
