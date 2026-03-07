#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{
    std::unique_ptr<Widget> Spacer(void)
    {
        return std::make_unique<Widget>();
    }

    std::unique_ptr<Widget> Spacer(int fs)
    {
        auto sp = std::make_unique<Widget>();
        sp->set_fixed_size(fs);
        return sp;
    }
} // namespace horizon