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

    uint32_t file_capabilities() const override { return FileAll; }
    std::string current_file_path() const override;
    std::vector<FileFilter> file_filters() const override;
    bool allows_multiple_open_files() const override { return true; }

    void open_file(const std::string& path);
    void new_file();
    void save_current_file();
    void update_status_bar();
    void load_settings();
    void focus_active_editor();

    /**
     * @brief Reads the language preference from config and loads the
     *        corresponding app locale before UI construction.
     * @return The selected locale code, or "default" if none.
     */
    std::string load_language_setting();

protected:
    void setup_ui();
    void create_tab(const std::string& title, std::shared_ptr<horizon::text::TextDocument> doc);
    std::string get_config_path();

private:
    TabCollection* m_tabs = nullptr;
    horizon::Label* m_status_label = nullptr;
    std::vector<std::shared_ptr<horizon::text::TextDocument>> m_documents;
};

} // namespace text_editor
} // namespace horizon
