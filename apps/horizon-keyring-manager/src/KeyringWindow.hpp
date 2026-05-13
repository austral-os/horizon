#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/Sidebar.hpp>
#include <horizon/TableView.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/VPanel.hpp>
#include <string>
#include <vector>

namespace horizon::keyring
{
    struct KeyringItem
    {
        std::string label;
        std::string type;
        std::string last_modified;
        std::string path;
    };

    class KeyringWindow : public ApplicationWindow
    {
    public:
        KeyringWindow(int w, int h);
        ~KeyringWindow() override = default;

    private:
        void setup_toolbar();
        void setup_content();
        void load_mock_data();

        Sidebar* m_sidebar{nullptr};
        TableView<KeyringItem>* m_table{nullptr};
        SearchBox* m_search_box{nullptr};
    };
}
