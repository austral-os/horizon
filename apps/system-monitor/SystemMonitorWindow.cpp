#include "SystemMonitorWindow.hpp"
#include "ProcessManager.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/Widget.hpp>
#include <iomanip>
#include <sstream>

namespace horizon
{
    SystemMonitorWindow::SystemMonitorWindow()
        : ApplicationWindow(i18n().tr("system_monitor.title"))
    {
        setup_toolbar();
        setup_content();

        // Initial update
        update_processes();
    }

    void SystemMonitorWindow::set_application_recursive(WaylandWindow *app)
    {
        ApplicationWindow::set_application_recursive(app);

        if (app && m_refresh_timer_id == 0)
        {
            m_refresh_timer_id = app->add_timer(1000, [this]() { update_processes(); }, true);
        }
    }

    void SystemMonitorWindow::setup_toolbar()
    {
        auto tb = toolbar();

        // Terminate Button
        auto btn_terminate = std::make_unique<ToolbarButton>(
            i18n().tr("system_monitor.toolbar.terminate"), "process-stop");
        m_btn_terminate = btn_terminate.get();
        m_btn_terminate->set_enabled(false);
        
        m_btn_terminate->when_click.connect(
            [this](EventContext &)
            {
                auto selected = m_table_view->get_selected_items();
                if (!selected.empty())
                {
                    const auto &p = selected[0];
                    std::string msg = i18n().tr("system_monitor.confirm_kill") + " " + p.name + " (PID: " + std::to_string(p.pid) + ")?";
                    if (application()->confirm(msg, i18n().tr("system_monitor.toolbar.terminate")))
                    {
                        ProcessManager pm;
                        pm.terminate_process(p.pid);
                        update_processes();
                    }
                }
            });
        tb->add_toolbar_widget(std::move(btn_terminate));

        // Information Button
        auto btn_info = std::make_unique<ToolbarButton>(i18n().tr("system_monitor.toolbar.info"),
                                                        "dialog-information");
        tb->add_toolbar_widget(std::move(btn_info));

        // Spacer
        tb->add_toolbar_widget(Spacer());

        // Group Button
        auto group_btn = std::make_unique<GroupButton>();
        group_btn->add_item("CPU");
        group_btn->add_item(i18n().tr("system_monitor.toolbar.memory"));
        group_btn->add_item(i18n().tr("system_monitor.toolbar.energy"));
        group_btn->add_item(i18n().tr("system_monitor.toolbar.disk"));
        group_btn->add_item(i18n().tr("system_monitor.toolbar.network"));
        group_btn->set_fixed_size(500);
        tb->add_toolbar_widget(std::move(group_btn));

        // Spacer
        tb->add_toolbar_widget(Spacer());

        // Search Box
        auto search_box = std::make_unique<SearchBox>();
        m_search_box = search_box.get();
        m_search_box->set_placeholder(i18n().tr("system_monitor.toolbar.search"));
        m_search_box->set_fixed_size(35);
        m_search_box->when_text_changed.connect([this](KeyEventContext &) { update_processes(); });

        auto search_wrapper = std::make_unique<Widget>();
        search_wrapper->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        search_wrapper->set_fixed_size(200);
        search_wrapper->add_child(std::move(search_box));
        tb->add_toolbar_widget(std::move(search_wrapper));
        tb->add_toolbar_widget(Spacer(10));
    }

    void SystemMonitorWindow::setup_content()
    {
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto table_view = std::make_unique<TableView<ProcessInfo>>();
        m_table_view = table_view.get();

        // Icon Column
        TableColumn<ProcessInfo> col_icon;
        col_icon.title = "";
        col_icon.width = 32;
        col_icon.sortable = false;
        col_icon.cell_factory = [](const ProcessInfo &p)
        {
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name(p.name);
            icon->set_icon_size(16);
            return icon;
        };
        m_table_view->add_column(col_icon);

        // PID Column
        TableColumn<ProcessInfo> col_pid;
        col_pid.title = "PID";
        col_pid.width = 80;
        col_pid.cell_factory = [](const ProcessInfo &p)
        { return std::make_unique<Label>(std::to_string(p.pid)); };
        col_pid.sort_predicate = [](const ProcessInfo &a, const ProcessInfo &b)
        { return a.pid < b.pid; };
        m_table_view->add_column(col_pid);

        // Process Name Column
        TableColumn<ProcessInfo> col_name;
        col_name.title = i18n().tr("system_monitor.columns.name");
        col_name.width = 200;
        col_name.cell_factory = [](const ProcessInfo &p)
        { return std::make_unique<Label>(p.name); };
        col_name.sort_predicate = [](const ProcessInfo &a, const ProcessInfo &b)
        { return a.name < b.name; };
        m_table_view->add_column(col_name);

        // Memory Column
        TableColumn<ProcessInfo> col_mem;
        col_mem.title = i18n().tr("system_monitor.columns.memory");
        col_mem.width = 100;
        col_mem.cell_factory = [](const ProcessInfo &p)
        {
            double mem_mb = p.memory_bytes / (1024.0 * 1024.0);
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << mem_mb << " MB";
            return std::make_unique<Label>(ss.str());
        };
        col_mem.sort_predicate = [](const ProcessInfo &a, const ProcessInfo &b)
        { return a.memory_bytes < b.memory_bytes; };
        m_table_view->add_column(col_mem);

        // % CPU Column
        TableColumn<ProcessInfo> col_cpu;
        col_cpu.title = "% CPU";
        col_cpu.width = 80;
        col_cpu.cell_factory = [](const ProcessInfo &p)
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << p.cpu_percent << "%";
            return std::make_unique<Label>(ss.str());
        };
        col_cpu.sort_predicate = [](const ProcessInfo &a, const ProcessInfo &b)
        { return a.cpu_percent < b.cpu_percent; };
        m_table_view->add_column(col_cpu);

        // User Column
        TableColumn<ProcessInfo> col_user;
        col_user.title = i18n().tr("system_monitor.columns.user");
        col_user.width = 100;
        col_user.cell_factory = [](const ProcessInfo &p)
        { return std::make_unique<Label>(p.user); };
        col_user.sort_predicate = [](const ProcessInfo &a, const ProcessInfo &b)
        { return a.user < b.user; };
        m_table_view->add_column(col_user);

        // Command Column
        TableColumn<ProcessInfo> col_cmd;
        col_cmd.title = i18n().tr("system_monitor.columns.command");
        col_cmd.width = 300;
        col_cmd.cell_factory = [](const ProcessInfo &p)
        { return std::make_unique<Label>(p.command); };
        col_cmd.sort_predicate = [](const ProcessInfo &a, const ProcessInfo &b)
        { return a.command < b.command; };
        m_table_view->add_column(col_cmd);

        m_table_view->when_row_click.connect([this](const TableViewRowMouseClickContext<ProcessInfo>& ctx) {
            m_selected_pid = ctx.row_data.pid;
            m_btn_terminate->set_enabled(true);
        });

        root->add_child(std::move(table_view));

        // Empty area for future graphs
        auto graphs_area = std::make_unique<Widget>();
        m_graphs_area = graphs_area.get();
        m_graphs_area->set_fixed_size(200); // 200px height for graphs
        m_graphs_area->set_position_type(FREE);
        root->add_child(std::move(graphs_area));

        set_content(std::move(root));
    }

    void SystemMonitorWindow::update_processes()
    {
        ProcessManager pm;
        auto processes = pm.get_processes();
        
        set_title(i18n().tr("system_monitor.title"));

        std::string filter = m_search_box->text();
        if (!filter.empty())
        {
            std::vector<ProcessInfo> filtered;
            for (const auto &p : processes)
            {
                if (p.name.find(filter) != std::string::npos ||
                    p.command.find(filter) != std::string::npos)
                {
                    filtered.push_back(p);
                }
            }
            m_table_view->set_data(std::move(filtered));
        }
        else
        {
            m_table_view->set_data(std::move(processes));
        }

        m_table_view->apply_sort();

        // Restore selection
        if (m_selected_pid != -1)
        {
            const auto &data = m_table_view->data();
            for (size_t i = 0; i < data.size(); ++i)
            {
                if (data[i].pid == m_selected_pid)
                {
                    m_table_view->set_selected_index((int)i);
                    m_btn_terminate->set_enabled(true);
                    return;
                }
            }
        }
        
        m_btn_terminate->set_enabled(false);
    }
} // namespace horizon
