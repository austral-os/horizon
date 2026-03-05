#include <horizon/ImageDriver.hpp>
#include <memory>

namespace horizon
{
    class SvgImageDriver : public ImageDriver
    {
    public:
        SvgImageDriver();
        ~SvgImageDriver();

        bool load(const std::string &path) override;
        void draw(GraphicsContext &ctx, int x, int y, int w, int h) override;
        int width() const override
        {
            return m_width;
        }
        int height() const override
        {
            return m_height;
        }

    private:
        int m_width{0};
        int m_height{0};

        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace horizon
