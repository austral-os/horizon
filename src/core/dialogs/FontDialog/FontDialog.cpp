#include "FontPreview.hpp"
#include <algorithm>
#include <fontconfig/fontconfig.h>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/TableView.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Window.hpp>
#include <horizon/dialogs/FontDialog.hpp>
#include <map>
#include <set>

namespace horizon
{

    struct FontInfo
    {
        std::string family;
        std::set<std::string> styles;
    };

    class FontDialog::Impl
    {
    public:
        Impl(FontDialog *p) : parent(p) {}

        void setup_ui(const std::string &title)
        {
            auto window_widget = std::make_unique<Window>(title);
            auto *app_window = window_widget.get();
            app_window->set_size(700, 550);

            auto root = std::make_unique<Widget>();
            root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            root->set_margin(12);
            root->set_spacing(12);

            // --- SECTOR A: Selección Principal (Horizontal) ---
            auto sector_a = std::make_unique<Widget>();
            sector_a->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            sector_a->set_spacing(12);
            sector_a->set_height(280);

            // Columna 1: Tipo de letra
            auto col1 = std::make_unique<Widget>();
            col1->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            col1->set_position_type(FILL);
            col1->set_height(250);
            col1->set_spacing(4);

            auto lbl1 = std::make_unique<Label>(i18n().tr("core.dialog.font.type_label"));
            lbl1->set_fixed_size(25);
            col1->add_child(std::move(lbl1));

            auto f_list = std::make_unique<TableView<std::string>>();
            family_list = f_list.get();
            family_list->set_header_visible(false);

            TableColumn<std::string> family_col;
            family_col.id = "family";
            family_col.title = i18n().tr("core.dialog.font.family");
            family_col.width_policy = ColumnWidthPolicy::Flexible;
            family_col.width = -1;
            family_col.cell_factory = [](const std::string &s)
            {
                auto lbl = std::make_unique<Label>(s);
                lbl->set_vertical_alignment(VerticalAlignment::Middle);
                lbl->set_margin(4);
                return lbl;
            };
            family_list->add_column(family_col);
            col1->add_child(std::move(f_list));
            sector_a->add_child(std::move(col1));

            // Columna 2: Estilo
            auto col2 = std::make_unique<Widget>();
            col2->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            col2->set_position_type(FILL);
            col2->set_height(250);
            col2->set_spacing(4);

            auto lbl2 = std::make_unique<Label>(i18n().tr("core.dialog.font.style_label"));
            lbl2->set_fixed_size(25);
            col2->add_child(std::move(lbl2));

            auto s_list = std::make_unique<TableView<std::string>>();
            style_list = s_list.get();
            style_list->set_header_visible(false);

            TableColumn<std::string> style_col;
            style_col.id = "style";
            style_col.title = i18n().tr("core.dialog.font.style");
            style_col.width_policy = ColumnWidthPolicy::Flexible;
            style_col.width = -1;
            style_col.cell_factory = [](const std::string &s)
            {
                auto lbl = std::make_unique<Label>(s);
                lbl->set_vertical_alignment(VerticalAlignment::Middle);
                lbl->set_margin(4);
                return lbl;
            };
            style_list->add_column(style_col);
            col2->add_child(std::move(s_list));
            sector_a->add_child(std::move(col2));

            // Columna 3: Tamaño
            auto col3 = std::make_unique<Widget>();
            col3->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            col3->set_spacing(4);
            col3->set_width(120);

            auto lbl3 = std::make_unique<Label>(i18n().tr("core.dialog.font.size_label"));
            lbl3->set_fixed_size(25);
            col3->add_child(std::move(lbl3));

            auto sz_in = std::make_unique<TextBox<>>();
            size_input = sz_in.get();
            size_input->set_text(std::to_string((int)selection_data.size));
            size_input->set_height(32);
            col3->add_child(std::move(sz_in));

            auto sz_list = std::make_unique<TableView<std::string>>();
            size_list = sz_list.get();
            size_list->set_header_visible(false);

            TableColumn<std::string> size_col;
            size_col.id = "size";
            size_col.title = i18n().tr("core.dialog.font.size");
            size_col.width_policy = ColumnWidthPolicy::Flexible;
            size_col.cell_factory = [](const std::string &s)
            {
                auto lbl = std::make_unique<Label>(s);
                lbl->set_vertical_alignment(VerticalAlignment::Middle);
                lbl->set_margin(4);
                return lbl;
            };
            size_list->add_column(size_col);
            col3->add_child(std::move(sz_list));
            sector_a->add_child(std::move(col3));

            root->add_child(std::move(sector_a));

            // --- SECTOR B: Previsualización y Características (Vertical) ---
            auto sector_b = std::make_unique<Widget>();
            sector_b->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            sector_b->set_spacing(8);

            auto sample_lbl = std::make_unique<Label>(i18n().tr("core.dialog.font.sample_label"));
            sample_lbl->set_fixed_size(25);
            sector_b->add_child(std::move(sample_lbl));

            auto preview = std::make_unique<FontPreview>();
            preview_widget = preview.get();
            sector_b->add_child(std::move(preview));

            root->add_child(std::move(sector_b));

            // --- SECTOR C: Barra de Estado y Botones (Horizontal) ---
            auto sector_c = std::make_unique<Widget>();
            sector_c->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            sector_c->set_fixed_size(40);
            sector_c->set_spacing(12);

            auto spacer = std::make_unique<Widget>();
            sector_c->add_child(std::move(spacer));

            auto ok_btn = std::make_unique<Button<AquaObject>>();
            ok_btn->set_text(i18n().tr("core.dialog.accept"));
            ok_btn->set_width(110);
            ok_btn->set_accent_color(WidgetAccentColor::Primary);
            ok_btn->when_mouse_press.connect(
                [this](MouseButtonEventContext &)
                {
                    FontDialogAcceptedContext ctx;
                    ctx.selection = selection_data;
                    parent->when_accepted.run(ctx);
                    parent->quit();
                });
            sector_c->add_child(std::move(ok_btn));

            auto cancel_btn = std::make_unique<Button<AquaObject>>();
            cancel_btn->set_text(i18n().tr("core.dialog.cancel"));
            cancel_btn->set_width(110);
            cancel_btn->when_mouse_press.connect(
                [this](MouseButtonEventContext &)
                {
                    FontDialogCancelledContext ctx;
                    parent->when_cancelled.run(ctx);
                    parent->quit();
                });
            sector_c->add_child(std::move(cancel_btn));

            root->add_child(std::move(sector_c));

            app_window->add_child(std::move(root));
            parent->set_root(std::move(window_widget));

            setup_signals();
            load_fonts();
        }

        void setup_signals()
        {
            family_list->when_row_click.connect(
                [this](TableViewRowMouseClickContext<std::string> &ctx)
                {
                    selection_data.family = ctx.row_data;
                    auto it = font_map.find(selection_data.family);
                    if (it != font_map.end())
                    {
                        std::vector<std::string> styles(it->second.styles.begin(),
                                                        it->second.styles.end());
                        style_list->set_data(styles);
                    }
                    update_preview();
                });

            style_list->when_row_click.connect(
                [this](TableViewRowMouseClickContext<std::string> &ctx)
                {
                    selection_data.style = ctx.row_data;
                    update_preview();
                });

            size_list->when_row_click.connect(
                [this](TableViewRowMouseClickContext<std::string> &ctx)
                {
                    selection_data.size = std::stof(ctx.row_data);
                    if (size_input)
                        size_input->set_text(std::to_string((int)selection_data.size));
                    update_preview();
                });

            if (size_input)
            {
                size_input->when_text_changed.connect(
                    [this](KeyEventContext &)
                    {
                        std::string val = size_input->text();
                        try
                        {
                            if (!val.empty())
                            {
                                selection_data.size = std::stof(val);
                                update_preview();
                                // Intentar seleccionar en la lista si coincide exactamente
                                auto data = size_list->data();
                                for (size_t i = 0; i < data.size(); ++i)
                                {
                                    if (data[i] == val)
                                    {
                                        size_list->set_selected_index(i);
                                        break;
                                    }
                                }
                            }
                        }
                        catch (...)
                        {
                        }
                    });
            }
        }

        void update_preview()
        {
            if (preview_widget)
            {
                preview_widget->set_font_family(selection_data.family);
                preview_widget->set_font_style(selection_data.style);
                preview_widget->set_font_size(selection_data.size);
            }
        }

        void load_fonts()
        {
            FcConfig *config = FcInitLoadConfigAndFonts();
            FcPattern *pat = FcPatternCreate();
            FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, NULL);
            FcFontSet *fs = FcFontList(config, pat, os);

            for (int i = 0; fs && i < fs->nfont; ++i)
            {
                FcPattern *font = fs->fonts[i];
                FcChar8 *family, *style;
                if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
                    FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch)
                {
                    font_map[reinterpret_cast<const char *>(family)].family =
                        reinterpret_cast<const char *>(family);
                    font_map[reinterpret_cast<const char *>(family)].styles.insert(
                        reinterpret_cast<const char *>(style));
                }
            }

            if (fs)
                FcFontSetDestroy(fs);
            FcObjectSetDestroy(os);
            FcPatternDestroy(pat);
            FcConfigDestroy(config);

            std::vector<std::string> families;
            for (auto const &[name, info] : font_map)
                families.push_back(name);

            LOG_INFO << "FontDialog: Loaded " << families.size() << " font families.";

            // Fallback if no fonts found
            if (families.empty())
            {
                families = {"Sans Serif", "Serif", "Monospace", "Arial", "Inter"};
                for (const auto &f : families)
                {
                    font_map[f].family = f;
                    font_map[f].styles = {"Regular", "Bold", "Italic"};
                }
            }

            std::sort(families.begin(), families.end());
            family_list->set_data(families);
            family_list->invalidate();

            std::vector<std::string> sizes = {"8",  "9",  "10", "11", "12", "13", "14",
                                              "15", "16", "18", "20", "22", "24", "26",
                                              "28", "32", "36", "48", "72"};
            size_list->set_data(sizes);
            size_list->invalidate();
        }

        FontDialog *parent;
        TableView<std::string> *family_list{nullptr};
        TableView<std::string> *style_list{nullptr};
        TableView<std::string> *size_list{nullptr};
        TextBox<> *size_input{nullptr};
        TextBox<> *features_input{nullptr};
        Checkbox<AquaObject> *show_all_fonts_cb{nullptr};
        FontPreview *preview_widget{nullptr};
        FontSelection selection_data{"Inter", "Regular", 12.0f, ""};
        std::map<std::string, FontInfo> font_map;
    };

    FontDialog::FontDialog(const std::string &title) : m_impl(std::make_unique<Impl>(this))
    {
        m_impl->setup_ui(title);
    }

    FontDialog::~FontDialog() = default;

    FontSelection FontDialog::selection() const
    {
        return m_impl->selection_data;
    }

    void FontDialog::set_selection(const FontSelection &sel)
    {
        m_impl->selection_data = sel;
    }

} // namespace horizon
