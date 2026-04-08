#include "horizon/Widget.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include <memory>

namespace horizon::arkfm
{

    enum class ViewMode
    {
        List,
        Grid,
        CoverFlow
    };

    struct PathChangedEvent : public EventContext
    {
        std::string path;
    };

    class ArkfmView : public Widget
    {
    public:
        ArkfmView(std::string path = ".");
        ~ArkfmView() override;

        void set_view_mode(ViewMode mode);

        void navigate_to(const std::string &path, bool record_history = true);
        void navigate_back();
        void navigate_forward();

        std::vector<arkutils::FileInfo> get_selection() const;
        void open_selection();
        void open_item(const arkutils::FileInfo &f);

        void set_search_query(const std::string &query);

        bool can_back() const;
        bool can_forward() const;

        const std::string &current_path() const;

        // Clipboard integration
        bool supports_clipboard() const override { return true; }
        bool can_perform(ClipboardAction action) const override;
        void perform(ClipboardAction action) override;
        void provide_clipboard_data(const std::string &mime, DataSink &sink) override;
        std::vector<std::string> provided_mime_types() const override;
        void on_clipboard_data_received(const std::string &mime, const std::vector<uint8_t> &data) override;

    protected:
        EventsManager<PathChangedEvent> when_path_changed;

    private:
        ViewMode m_view_mode;
        std::string m_current_path;
        std::string m_search_query;
        std::unique_ptr<class NavigationHistory> m_history;

        // Clipboard state
        std::vector<std::string> m_clipboard_paths;
        bool m_is_cut = false;
    };

} // namespace horizon::arkfm