#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/RadioButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/VPanel.hpp>
#include <iostream>
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
    }

    void SoundView::setup_output_tab(Widget *container)
    {
        auto label_devices =
            std::make_unique<Label>("Dispositivos de salida de audio disponibles:");
        label_devices->set_fixed_size(25);
        label_devices->set_font_weight(FONT_WEIGHT_BOLD);
        container->add_child(std::move(label_devices));

        // TableView for Output Devices
        auto table = std::make_unique<TableView<AudioDevice>>();
        m_output_table = table.get();
        m_output_table->set_fixed_size(200);
        m_output_table->set_header_visible(false);

        TableColumn<AudioDevice> col_sel;
        col_sel.id = "selected";
        col_sel.width = 40;
        col_sel.cell_factory = [](const AudioDevice &dev) -> std::unique_ptr<Widget>
        {
            auto rb = std::make_unique<RadioButton<AquaObject>>();
            rb->set_selected(dev.is_default);
            // rb->set_enabled(false);
            return std::unique_ptr<Widget>(rb.release());
        };
        m_output_table->add_column(col_sel);

        TableColumn<AudioDevice> col_desc;
        col_desc.id = "description";
        col_desc.width = 400;
        col_desc.cell_factory = [](const AudioDevice &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>(dev.description);
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_output_table->add_column(col_desc);

        TableColumn<AudioDevice> col_type;
        col_type.id = "type";
        col_type.width = 100;
        col_type.cell_factory = [](const AudioDevice &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>("Salida");
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            lbl->set_alignment(TextAlignment::Right);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_output_table->add_column(col_type);

        m_output_table->when_row_click.connect(
            [this](TableViewRowMouseClickContext<AudioDevice> &ctx)
            { PipeWireManager::instance().set_default(ctx.row_data.id); });

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

        auto table = std::make_unique<TableView<AudioDevice>>();
        m_input_table = table.get();
        m_input_table->set_fixed_size(200);
        m_input_table->set_header_visible(false);

        TableColumn<AudioDevice> col_sel;
        col_sel.id = "selected";
        col_sel.width = 40;
        col_sel.cell_factory = [](const AudioDevice &dev) -> std::unique_ptr<Widget>
        {
            auto rb = std::make_unique<RadioButton<AquaObject>>();
            rb->set_selected(dev.is_default);
            return std::unique_ptr<Widget>(rb.release());
        };
        m_input_table->add_column(col_sel);

        TableColumn<AudioDevice> col_desc;
        col_desc.id = "description";
        col_desc.width = 400;
        col_desc.cell_factory = [](const AudioDevice &dev) -> std::unique_ptr<Widget>
        {
            auto lbl = std::make_unique<Label>(dev.description);
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            return std::unique_ptr<Widget>(lbl.release());
        };
        m_input_table->add_column(col_desc);

        TableColumn<AudioDevice> col_type;
        col_type.id = "type";
        col_type.width = 100;
        col_type.cell_factory = [](const AudioDevice &dev) -> std::unique_ptr<Widget>
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
        m_input_volume_slider->set_min(0.0f);
        m_input_volume_slider->set_max(1.0f);
        m_input_volume_slider->set_value(0.5f);
        vol_panel->add_child(std::move(slider));
        container->add_child(std::move(vol_panel));
    }

    void SoundView::update_device_list()
    {
        if (m_output_table)
        {
            m_output_table->set_data(PipeWireManager::instance().get_sinks());
        }
        if (m_input_table)
        {
            m_input_table->set_data(PipeWireManager::instance().get_sources());
        }
    }

    void SoundView::on_volume_slider_changed(float value)
    {
        // For now, change volume of all sinks (or default if we implement default tracking)
        auto sinks = PipeWireManager::instance().get_sinks();
        for (const auto &sink : sinks)
        {
            if (sink.is_default)
            {
                PipeWireManager::instance().set_volume(sink.id, value);
            }
        }
    }

    void SoundView::on_balance_slider_changed(float value)
    {
        auto sinks = PipeWireManager::instance().get_sinks();
        for (const auto &sink : sinks)
        {
            if (sink.is_default)
            {
                PipeWireManager::instance().set_balance(sink.id, value);
            }
        }
    }

    void SoundView::test_speakers()
    {
        std::system("pw-play /usr/share/sounds/alsa/Front_Center.wav &");
    }
} // namespace horizon::preferences
