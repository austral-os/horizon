#include <views/WifiView/WifiView.hpp>
#include <horizon/Notebook.hpp>
#include <views/WifiView/WifiConfigView.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    WifiView::WifiView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(0);
        set_spacing(0);

        auto notebook = std::make_unique<Notebook>();
        
        // Tab 1: Wi-Fi
        auto wifi_config = std::make_unique<WifiConfigView>();
        m_wifi_config = wifi_config.get();
        notebook->add_tab(NotebookPage("Wi-Fi", std::move(wifi_config)));

        // Tab 2: TCP/IP
        auto tcp_ip_view = std::make_unique<Widget>();
        tcp_ip_view->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        tcp_ip_view->set_margin(20);
        tcp_ip_view->add_child(std::make_unique<Label>("Configuración TCP/IP (Próximamente)"));
        notebook->add_tab(NotebookPage("TCP/IP", std::move(tcp_ip_view)));

        m_notebook = notebook.get();
        add_child(std::move(notebook));
    }
}
