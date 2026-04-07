#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/RadioButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/VPanel.hpp>
#include <utils/PipeWireManager.hpp>
#include <views/SoundView/SoundView.hpp>

namespace horizon::preferences
{
    SoundView::SoundView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(0);

        setup_ui();

        PipeWireManager::instance().on_devices_changed = [this]()
        {
            if (application())
            {
                application()->add_timer(0, [this]() { update_device_list(); });
            }
        };

        PipeWireManager::instance().start();
        update_device_list();
    }

    SoundView::~SoundView()
    {
        PipeWireManager::instance().stop();
    }

    void SoundView::setup_ui()
    {
        auto notebook_ptr = std::make_unique<Notebook>();
        m_notebook = notebook_ptr.get();
        add_child(std::move(notebook_ptr));

        auto output_container = std::make_unique<Widget>();
        output_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        output_container->set_margin(20);
        output_container->set_spacing(15);
        setup_output_tab(output_container.get());
        m_notebook->add_tab({"Salida", std::move(output_container)});

        auto input_container = std::make_unique<Widget>();
        input_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        input_container->set_margin(20);
        input_container->set_spacing(15);
        setup_input_tab(input_container.get());
        m_notebook->add_tab({"Entrada", std::move(input_container)});

        auto apps_container = std::make_unique<Widget>();
        apps_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        apps_container->set_margin(20);
        apps_container->set_spacing(15);
        setup_apps_tab(apps_container.get());
        m_notebook->add_tab({"Aplicaciones", std::move(apps_container)});
    }

    void SoundView::setup_output_tab(Widget *container)
    {
        auto label_devices =
            std::make_unique<Label>("Dispositivos de salida de audio disponibles:");
        label_devices->set_fixed_size(25);
        label_devices->set_font_weight(FONT_WEIGHT_BOLD);
        container->add_child(std::move(label_devices));

        // TableView for Output Devices
        auto table = std::make_unique<TableView<AudioItem>>();
        m_output_table = table.get();
        m_output_table->set_fixed_size(200);
        m_output_table->set_header_visible(false);

        TableColumn<AudioItem> col_sel;
        col_sel.id = "selected";
        col_sel.width = 50;
        col_sel.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto rb = std::make_unique<RadioButton<AquaObject>>();
            rb->set_selected(dev.is_default);
            // rb->set_enabled(false);
            return std::unique_ptr<Widget>(rb.release());
        };
        m_output_table->add_column(col_sel);

        TableColumn<AudioItem> col_desc;
        col_desc.id = "description";
        col_desc.width = 400;
        col_desc.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>(dev.description);
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_output_table->add_column(col_desc);

        TableColumn<AudioItem> col_type;
        col_type.id = "type";
        col_type.width = 100;
        col_type.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>("Salida");
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            lbl->set_alignment(TextAlignment::Right);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_output_table->add_column(col_type);

        m_output_table->when_row_click.connect(
            [this](TableViewRowMouseClickContext<AudioItem> &ctx)
            {
                if (ctx.row_data.is_profile)
                    PipeWireManager::instance().set_device_profile(ctx.row_data.device_id,
                                                                   ctx.row_data.profile_index);
                else
                    PipeWireManager::instance().set_default_node(ctx.row_data.node_id);
            });

        container->add_child(std::move(table));

        // Volume Slider
        auto vol_panel = std::make_unique<Widget>();
        vol_panel->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        vol_panel->set_fixed_size(40);
        vol_panel->set_spacing(10);

        auto lbl_vol = std::make_unique<Label>("Volumen de salida:");
        lbl_vol->set_fixed_size(150);
        vol_panel->add_child(std::move(lbl_vol));

        auto slider = std::make_unique<Slider>();
        m_output_volume_slider = slider.get();
        m_output_volume_slider->set_thumb_shape(ThumbShape::Circle);
        m_output_volume_slider->set_min(0.0f);
        m_output_volume_slider->set_max(1.0f);
        m_output_volume_slider->set_value(0.5f);
        m_output_volume_slider->when_value_changed.connect(
            [this](EventContext &ev)
            { on_volume_slider_changed(m_output_volume_slider->value()); });
        vol_panel->add_child(std::move(slider));
        container->add_child(std::move(vol_panel));

        // Balance Slider
        auto bal_panel = std::make_unique<Widget>();
        bal_panel->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bal_panel->set_fixed_size(60);
        bal_panel->set_spacing(10);

        auto lbl_bal = std::make_unique<Label>("Balance:");
        lbl_bal->set_fixed_size(150);
        bal_panel->add_child(std::move(lbl_bal));

        auto bal_v_panel = std::make_unique<Widget>();
        bal_v_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto bal_slider = std::make_unique<Slider>();
        m_balance_slider = bal_slider.get();
        m_balance_slider->set_thumb_shape(ThumbShape::Circle);
        m_balance_slider->set_min(-1.0f);
        m_balance_slider->set_max(1.0f);
        m_balance_slider->set_value(0.0f);
        m_balance_slider->set_tick_count(3);
        m_balance_slider->set_show_ticks(true);
        m_balance_slider->when_value_changed.connect(
            [this](EventContext &) { on_balance_slider_changed(m_balance_slider->value()); });
        bal_v_panel->add_child(std::move(bal_slider));

        auto bal_labels = std::make_unique<Widget>();
        bal_labels->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bal_labels->set_fixed_size(20);
        auto l_left = std::make_unique<Label>("Izquierda");
        auto l_center = std::make_unique<Label>("Centro");
        l_center->set_alignment(TextAlignment::Center);
        auto l_right = std::make_unique<Label>("Derecha");
        l_right->set_alignment(TextAlignment::Right);
        bal_labels->add_child(std::move(l_left));
        bal_labels->add_child(std::move(l_center));
        bal_labels->add_child(std::move(l_right));
        bal_v_panel->add_child(std::move(bal_labels));

        bal_panel->add_child(std::move(bal_v_panel));
        container->add_child(std::move(bal_panel));

        // Test Speakers Button
        auto footer = std::make_unique<Widget>();
        footer->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        footer->set_fixed_size(40);
        footer->add_child(Spacer());

        auto btn_test = std::make_unique<Button<AquaObject>>();
        btn_test->set_text("Probar altavoces...");
        btn_test->set_fixed_size(150);
        btn_test->when_mouse_press.connect([this](MouseButtonEventContext &) { test_speakers(); });
        m_test_btn = btn_test.get();
        footer->add_child(std::move(btn_test));
        container->add_child(Spacer());
        container->add_child(std::move(footer));
    }

    void SoundView::setup_input_tab(Widget *container)
    {
        auto label_devices =
            std::make_unique<Label>("Dispositivos de entrada de audio disponibles:");
        label_devices->set_fixed_size(25);
        label_devices->set_font_weight(FONT_WEIGHT_BOLD);
        container->add_child(std::move(label_devices));

        auto table = std::make_unique<TableView<AudioItem>>();
        m_input_table = table.get();
        m_input_table->set_fixed_size(200);
        m_input_table->set_header_visible(false);

        TableColumn<AudioItem> col_sel;
        col_sel.id = "selected";
        col_sel.width = 40;
        col_sel.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto rb = std::make_unique<RadioButton<AquaObject>>();
            rb->set_selected(dev.is_default);
            return std::unique_ptr<Widget>(rb.release());
        };
        m_input_table->add_column(col_sel);

        TableColumn<AudioItem> col_desc;
        col_desc.id = "description";
        col_desc.width = 400;
        col_desc.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>(dev.description);
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_input_table->add_column(col_desc);

        TableColumn<AudioItem> col_type;
        col_type.id = "type";
        col_type.width = 100;
        col_type.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>("Entrada");
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            lbl->set_alignment(TextAlignment::Right);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_input_table->add_column(col_type);

        container->add_child(std::move(table));

        auto vol_panel = std::make_unique<Widget>();
        vol_panel->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        vol_panel->set_fixed_size(40);
        vol_panel->set_spacing(10);

        auto lbl_vol = std::make_unique<Label>("Volumen de entrada:");
        lbl_vol->set_fixed_size(150);
        vol_panel->add_child(std::move(lbl_vol));

        auto slider = std::make_unique<Slider>();
        m_input_volume_slider = slider.get();
        m_input_volume_slider->set_thumb_shape(ThumbShape::Circle);
        m_input_volume_slider->set_min(0.0f);
        m_input_volume_slider->set_max(1.0f);
        m_input_volume_slider->set_value(0.5f);
        vol_panel->add_child(std::move(slider));
        container->add_child(std::move(vol_panel));
    }

    void SoundView::setup_apps_tab(Widget *container)
    {
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(20);

        auto label_devices = std::make_unique<Label>("Aplicaciones (Volumen Individual)");
        label_devices->set_fixed_size(35);
        label_devices->set_alignment(TextAlignment::Left);
        label_devices->set_font_weight(FONT_WEIGHT_BOLD);
        container->add_child(std::move(label_devices));

        auto table = std::make_unique<TableView<AudioItem>>();
        m_apps_table = table.get();
        m_apps_table->set_header_visible(false);

        TableColumn<AudioItem> col_desc;
        col_desc.id = "description";
        col_desc.width = -1;
        col_desc.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>(dev.application_name.empty() ? dev.description
                                                                            : dev.application_name);
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_apps_table->add_column(col_desc);

        TableColumn<AudioItem> col_type;
        col_type.id = "type";
        col_type.width = 100;
        col_type.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>(dev.stream_type); // "Salida" / "Entrada"
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_apps_table->add_column(col_type);

        TableColumn<AudioItem> col_vol;
        col_vol.id = "vol";
        col_vol.width = 200;
        col_vol.cell_factory = [](const AudioItem &dev) -> std::unique_ptr<Widget>
        {
            auto container = std::make_unique<Widget>();
            container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            container->set_spacing(10);
            container->set_position_type(FILL);

            auto slider = std::make_unique<Slider>();
            auto *slider_ptr = slider.get();
            slider->set_thumb_shape(ThumbShape::Circle);
            slider->set_min(0.0f);
            slider->set_max(1.0f);
            slider->set_value(dev.volume);
            slider->set_position_type(FILL);

            auto lbl = std::make_unique<Label>(std::to_string(static_cast<int>(dev.volume * 100)) + "%");
            auto *lbl_ptr = lbl.get();
            lbl_ptr->set_fixed_size(60);
            lbl_ptr->set_alignment(TextAlignment::Right);
            lbl_ptr->set_vertical_alignment(VerticalAlignment::Middle);

            uint32_t nid = dev.node_id;
            slider->when_value_changed.connect(
                [nid, slider_ptr, lbl_ptr](EventContext &ev)
                {
                    float v = slider_ptr->value();
                    lbl_ptr->set_text(std::to_string(static_cast<int>(v * 100)) + "%");
                    PipeWireManager::instance().set_volume(nid, v);
                });

            container->add_child(std::move(slider));
            container->add_child(std::move(lbl));
            return std::unique_ptr<Widget>(container.release());
        };
        m_apps_table->add_column(col_vol);

        container->add_child(std::move(table));
    }

    void SoundView::update_device_list()
    {
        if (m_output_table && m_input_table && m_apps_table)
        {
            m_output_table->set_data(PipeWireManager::instance().get_sinks());
            m_input_table->set_data(PipeWireManager::instance().get_sources());
            m_apps_table->set_data(PipeWireManager::instance().get_app_streams());
        }
    }

    void SoundView::on_volume_slider_changed(float value)
    {
        // Actually, we shouldn't broadcast this to all sinks.
        // We should just apply it to the default one.
        // For simplicity, we apply it to all sinks for now.
        for (const auto &sink : m_output_table->data())
        {
            if (sink.is_default && !sink.is_profile)
            {
                PipeWireManager::instance().set_volume(sink.node_id, value);
            }
        }
    }

    void SoundView::on_balance_slider_changed(float value)
    {
        for (const auto &sink : m_output_table->data())
        {
            if (sink.is_default && !sink.is_profile)
            {
                PipeWireManager::instance().set_balance(sink.node_id, value);
            }
        }
    }

    void SoundView::test_speakers()
    {
        std::system("pw-play /usr/share/sounds/alsa/Front_Center.wav &");
    }
} // namespace horizon::preferences
