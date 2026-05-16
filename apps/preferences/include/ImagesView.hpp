#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Image.hpp>
#include <horizon/EventsManager.hpp>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace horizon::preferences
{
    /**
     * @class ImagesView
     * @brief A widget that displays a grid of image thumbnails loaded asynchronously.
     */
    class ImagesView : public Widget
    {
    public:
        ImagesView();
        ~ImagesView() override;
        void calculate_layout() override;
        void set_application_recursive(WaylandWindow *app) override;

        /**
         * @brief Set the directory path to load images from.
         * @param path The absolute directory path.
         */
        void set_path(const std::string& path);

        /**
         * @brief Event emitted when an image is selected.
         * The parameter is the absolute path to the selected ORIGINAL image.
         */
        EventsManager<const std::string&> when_image_selected;

    protected:
        void update_layout();

    private:
        void start_loading(const std::string& path);
        void stop_loading();
        void add_thumbnail_safe(const std::string& original_path, const std::string& thumbnail_path);

    private:
        ScrollArea* m_scroll_area{nullptr};
        Widget* m_grid_container{nullptr};
        
        std::string m_current_path;
        std::thread m_loading_thread;
        std::atomic<bool> m_stop_requested{false};
        std::vector<std::string> m_image_paths;
        
        static constexpr int THUMB_SIZE = 120;
        static constexpr int SPACING = 10;

        struct PendingThumbnail {
            std::string original;
            std::string thumb;
        };
        std::vector<PendingThumbnail> m_pending_thumbnails;
        std::mutex m_pending_mutex;
        std::shared_ptr<std::atomic<bool>> m_alive{std::make_shared<std::atomic<bool>>(true)};
    };

    /**
     * @internal
     * Helper widget for each thumbnail in the grid.
     */
    class ThumbnailItem : public Widget
    {
    public:
        ThumbnailItem(const std::string& original_path, const std::string& thumbnail_path);
        void draw(horizon::GraphicsContext& gc) override;
        
        const std::string& original_path() const { return m_original_path; }

    private:
        std::string m_original_path;
        Image* m_image{nullptr};
    };
} // namespace horizon::preferences
