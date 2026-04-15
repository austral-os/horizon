#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/TabCollection.hpp>
#include <horizon/Label.hpp>
#include <horizon/text/TextEditorWidget.hpp>
#include <memory>
#include <vector>

namespace horizon {
namespace text_editor {

class TextEditorWindow : public ApplicationWindow {
public:
    TextEditorWindow();
    virtual ~TextEditorWindow() = default;

    void open_file(const std::string& path);
    void new_file();
    void save_current_file();
    void update_status_bar();

protected:
    void setup_ui();
    void create_tab(const std::string& title, std::shared_ptr<horizon::text::TextDocument> doc);

private:
    TabCollection* m_tabs = nullptr;
    horizon::Label* m_status_label = nullptr;
    std::vector<std::shared_ptr<horizon::text::TextDocument>> m_documents;
};

} // namespace text_editor
} // namespace horizon
