#include "DownloadsPage.hpp"
#include "horizon/Application.hpp"
#include "horizon/GroupButton.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Panel.hpp"
#include "horizon/ProgressBar.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/TableColumn.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/download/DownloadManager.hpp"
#include <iomanip>
#include <sstream>

namespace horizon
{
    namespace nova
    {

        // Helper to format sizes
        static std::string format_size(size_t bytes)
        {
            if (bytes == 0)
                return "0 B";
            const char *units[] = {"B", "KB", "MB", "GB", "TB"};
            int i = 0;
            double d_bytes = (double)bytes;
            while (d_bytes >= 1024 && i < 4)
            {
                d_bytes /= 1024;
                i++;
            }
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << d_bytes << " " << units[i];
            return ss.str();
        }

        // Internal cell widget for progress
        class TaskProgressCell : public horizon::Widget
        {
        public:
            TaskProgressCell(std::shared_ptr<download::DownloadTask> task) : m_task(task)
            {
                set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
                set_margin(12);

                auto pb = std::make_unique<horizon::ProgressBar>();
                m_pb = pb.get();
                m_pb->set_height(6);
                m_pb->set_progress((float)m_task->progress().progress);
                add_child(std::move(pb));

                m_conn_id = m_task->when_progress_changed.connect(
                    [this](download::DownloadProgress &p)
                    {
                        if (application())
                        {
                            application()->post_task(
                                [this, p]()
                                {
                                    if (application() && application()->is_widget_alive(this))
                                    {
                                        m_pb->set_progress((float)p.progress);
                                    }
                                });
                        }
                    });
            }

            ~TaskProgressCell() override
            {
                if (m_task && m_conn_id != 0)
                {
                    m_task->when_progress_changed.disconnect(m_conn_id);
                }
            }

        private:
            std::shared_ptr<download::DownloadTask> m_task;
            horizon::ProgressBar *m_pb;
            size_t m_conn_id = 0;
        };

        // Internal cell widget for various text details
        class TaskDetailCell : public horizon::Widget
        {
        public:
            enum Field
            {
                Percentage,
                Speed,
                ETA,
                Status
            };
            TaskDetailCell(std::shared_ptr<download::DownloadTask> task, Field field)
                : m_task(task), m_field(field)
            {
                set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
                set_margin(12);

                auto lbl = std::make_unique<horizon::Label>("");
                m_lbl = lbl.get();
                m_lbl->set_alignment(horizon::TextAlignment::Left);
                add_child(std::move(lbl));

                auto update_fn = [this]()
                {
                    if (application())
                    {
                        application()->post_task(
                            [this]()
                            {
                                if (!application() || !application()->is_widget_alive(this))
                                    return;

                                auto p = m_task->progress();
                                std::string text;
                                switch (m_field)
                                {
                                case Percentage:
                                    text = std::to_string((int)(p.progress * 100)) + "%";
                                    break;
                                case Speed:
                                    text = format_size((size_t)p.speed) + "/s";
                                    break;
                                case ETA:
                                    if (p.eta_seconds > 0 &&
                                        m_task->state() == download::DownloadState::DOWNLOADING)
                                    {
                                        if (p.eta_seconds > 3600)
                                            text = std::to_string(p.eta_seconds / 3600) + "h " +
                                                   std::to_string((p.eta_seconds % 3600) / 60) +
                                                   "m";
                                        else if (p.eta_seconds > 60)
                                            text = std::to_string(p.eta_seconds / 60) + "m " +
                                                   std::to_string(p.eta_seconds % 60) + "s";
                                        else
                                            text = std::to_string(p.eta_seconds) + "s";
                                    }
                                    else
                                    {
                                        text = "--";
                                    }
                                    break;
                                case Status:
                                    switch (m_task->state())
                                    {
                                    case download::DownloadState::PENDING:
                                        text = "Pendiente";
                                        break;
                                    case download::DownloadState::DOWNLOADING:
                                        text = "Descargando";
                                        break;
                                    case download::DownloadState::PAUSED:
                                        text = "Pausado";
                                        break;
                                    case download::DownloadState::COMPLETED:
                                        text = "Completado";
                                        break;
                                    case download::DownloadState::FAILED:
                                        text = "Error";
                                        break;
                                    case download::DownloadState::CANCELLED:
                                        text = "Cancelado";
                                        break;
                                    }
                                    break;
                                }
                                m_lbl->set_text(text);
                            });
                    }
                };

                m_conn_state = m_task->when_state_changed.connect(
                    [update_fn](download::DownloadState) { update_fn(); });
                m_conn_prog = m_task->when_progress_changed.connect(
                    [update_fn](download::DownloadProgress &) { update_fn(); });

                update_fn();
            }

            ~TaskDetailCell() override
            {
                if (m_task)
                {
                    m_task->when_state_changed.disconnect(m_conn_state);
                    m_task->when_progress_changed.disconnect(m_conn_prog);
                }
            }

        private:
            std::shared_ptr<download::DownloadTask> m_task;
            Field m_field;
            horizon::Label *m_lbl;
            size_t m_conn_state = 0;
            size_t m_conn_prog = 0;
        };

        DownloadsPage::DownloadsPage()
        {
            set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
            set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
            setup_ui();
        }

        void DownloadsPage::setup_ui()
        {

            // 2. Table
            auto table =
                std::make_unique<horizon::TableView<std::shared_ptr<download::DownloadTask>>>();
            m_table = table.get();
            m_table->set_header_visible(true);
            m_table->set_row_height(50);
            m_table->set_margin(20);

            // Icon
            horizon::TableColumn<std::shared_ptr<download::DownloadTask>> col_icon;
            col_icon.id = "icon";
            col_icon.title = "";
            col_icon.width = 40;
            col_icon.cell_factory = [](const std::shared_ptr<download::DownloadTask> &task)
            {
                auto icon = std::make_unique<horizon::Icon>();
                icon->set_icon_name("folder-download-symbolic");
                icon->set_icon_size(24);
                return icon;
            };
            m_table->add_column(std::move(col_icon));

            // Filename
            horizon::TableColumn<std::shared_ptr<download::DownloadTask>> col_name;
            col_name.id = "name";
            col_name.title = "Nombre";
            col_name.width = 300;
            col_name.width_policy = horizon::ColumnWidthPolicy::Flexible;
            col_name.cell_factory = [](const std::shared_ptr<download::DownloadTask> &task)
            {
                auto lbl = std::make_unique<horizon::Label>(task->filename());
                lbl->set_alignment(horizon::TextAlignment::Left);
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
                return lbl;
            };
            m_table->add_column(std::move(col_name));

            // Status
            horizon::TableColumn<std::shared_ptr<download::DownloadTask>> col_status;
            col_status.id = "status";
            col_status.title = "Estado";
            col_status.width = 120;
            col_status.cell_factory = [](const std::shared_ptr<download::DownloadTask> &task)
            { return std::make_unique<TaskDetailCell>(task, TaskDetailCell::Status); };
            m_table->add_column(std::move(col_status));

            // Progress
            horizon::TableColumn<std::shared_ptr<download::DownloadTask>> col_progress;
            col_progress.id = "progress";
            col_progress.title = "Progreso";
            col_progress.width = 150;
            col_progress.cell_factory = [](const std::shared_ptr<download::DownloadTask> &task)
            { return std::make_unique<TaskProgressCell>(task); };
            m_table->add_column(std::move(col_progress));

            // %
            horizon::TableColumn<std::shared_ptr<download::DownloadTask>> col_pct;
            col_pct.id = "pct";
            col_pct.title = "%";
            col_pct.width = 60;
            col_pct.cell_factory = [](const std::shared_ptr<download::DownloadTask> &task)
            { return std::make_unique<TaskDetailCell>(task, TaskDetailCell::Percentage); };
            m_table->add_column(std::move(col_pct));

            // Speed
            horizon::TableColumn<std::shared_ptr<download::DownloadTask>> col_speed;
            col_speed.id = "speed";
            col_speed.title = "Velocidad";
            col_speed.width = 100;
            col_speed.cell_factory = [](const std::shared_ptr<download::DownloadTask> &task)
            { return std::make_unique<TaskDetailCell>(task, TaskDetailCell::Speed); };
            m_table->add_column(std::move(col_speed));

            // ETA
            horizon::TableColumn<std::shared_ptr<download::DownloadTask>> col_eta;
            col_eta.id = "eta";
            col_eta.title = "Tiempo";
            col_eta.width = 100;
            col_eta.cell_factory = [](const std::shared_ptr<download::DownloadTask> &task)
            { return std::make_unique<TaskDetailCell>(task, TaskDetailCell::ETA); };
            m_table->add_column(std::move(col_eta));

            add_child(std::move(table));

            download::DownloadManager::instance().when_task_added.connect(
                [this](std::shared_ptr<download::DownloadTask>)
                {
                    if (application())
                    {
                        application()->post_task([this]() { this->refresh_list(); });
                    }
                });

            refresh_list();
        }

        void DownloadsPage::refresh_list()
        {
            m_table->set_data(download::DownloadManager::instance().tasks());
        }

        void DownloadsPage::calculate_layout()
        {
            if (m_children.size() >= 2)
            {
                m_children[0]->set_height(48);
                m_children[1]->set_height(height() - 48);
            }
            horizon::Widget::calculate_layout();
        }

    } // namespace nova
} // namespace horizon
