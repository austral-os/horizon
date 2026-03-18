#include "horizon/Widget.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/arkutils/FileSystemModel.hpp"
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

        void set_search_query(const std::string &query);

        bool can_back() const;
        bool can_forward() const;

        const std::string &current_path() const;

        EventsManager<PathChangedEvent> when_path_changed;

    private:
        ViewMode m_view_mode;
        std::string m_current_path;
        std::string m_search_query;
        std::unique_ptr<class NavigationHistory> m_history;
    };

} // namespace horizon::arkfm