#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/apt/AppStoreClient.hpp>
#include <horizon/CoverFlow.hpp>

namespace horizon::appstore {

class FeaturedView : public horizon::Widget {
public:
    FeaturedView();
    ~FeaturedView() override;

    void load_initial_data();

    struct AppClickedContext {
        std::string package_name;
    };
    horizon::EventsManager<AppClickedContext> when_app_clicked;

private:
    void setup_ui();
    
    horizon::ScrollArea* m_scroll_area = nullptr;
    horizon::apt::AppStoreClient m_api_client;
    
    // Pointers to the internal components of the banner
    horizon::CoverFlow<horizon::apt::FeaturedApp>* m_coverflow = nullptr;
    class FeaturedBannerWidget* m_banner = nullptr;
    
    std::shared_ptr<bool> m_is_alive;
    bool m_data_loaded = false;
};

}
