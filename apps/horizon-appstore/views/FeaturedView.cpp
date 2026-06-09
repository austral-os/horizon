#include "FeaturedView.hpp"
#include <horizon/UnderConstruction.hpp>

namespace horizon::appstore {

FeaturedView::FeaturedView() {
    set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
    
    auto construction = std::make_unique<horizon::UnderConstruction>();
    add_child(std::move(construction));
}

}
