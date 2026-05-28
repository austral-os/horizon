#include "horizon/files/FileView.hpp"
#include "horizon/files/FileCoverFlowView.hpp"
#include "horizon/files/FileIconView.hpp"
#include "horizon/files/FileListView.hpp"
#include "horizon/files/FileHistory.hpp"
#include <horizon/Logger.hpp>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/Clipboard.hpp>
#include <horizon/ClipboardProvider.hpp>
#include <horizon/arkutils/FileOperations.hpp>
#include <horizon/NotificationSender.hpp>
#include <sys/stat.h>
#include <filesystem>
#include <sstream>
#include <thread>

namespace horizon::files
{
    FileView::FileView(std::string path)
        : Widget(), m_current_path(std::move(path)),
          m_history(std::make_unique<FileHistory>())
    {
        m_history->push(m_current_path);
        set_focusable(true);
        set_view_mode(ViewMode::List);
    }

    FileView::~FileView() = default;

    void FileView::refresh()
    {
        set_view_mode(m_view_mode);
    }

    void FileView::set_view_mode(ViewMode vm)
    {
        m_view_mode = vm;
        clear_children();
        LOG_INFO << "FileView [" << (void*)this << "]: Children cleared. Remaining: " << m_children.size() << ". Mode: " << (int)vm;

        Widget* new_view_ptr = nullptr;

        if (m_view_mode == ViewMode::List)
        {
            auto view = std::make_unique<FileListView>(m_current_path);
            view->set_show_hidden_files(m_show_hidden_files);
            view->set_file_filter(m_file_filter);
            view->when_row_dbl_click.connect(
                [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.row_data);
                });
            view->when_operation_progress.connect(
                [this](OperationProgressEvent &ctx)
                {
                    this->when_operation_progress.run(ctx);
                });

            if (m_context_menu_factory)
            {
                view->set_context_menu_factory(m_context_menu_factory);
            }
            new_view_ptr = view.get();
            add_child(std::move(view));
        }
        else if (m_view_mode == ViewMode::Grid)
        {
            auto view = std::make_unique<FileIconView>(m_current_path);
            view->set_show_hidden_files(m_show_hidden_files);
            view->set_file_filter(m_file_filter);
            view->when_item_dbl_click.connect(
                [this](horizon::IconViewItemMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.item_data);
                });
            view->when_operation_progress.connect(
                [this](OperationProgressEvent &ctx)
                {
                    this->when_operation_progress.run(ctx);
                });
            if (m_context_menu_factory)
            {
                view->set_context_menu_factory(m_context_menu_factory);
            }
            new_view_ptr = view.get();
            add_child(std::move(view));
        }
        else if (m_view_mode == ViewMode::CoverFlow)
        {
            auto view = std::make_unique<FileCoverFlowView>(m_current_path);
            view->set_show_hidden_files(m_show_hidden_files);
            view->set_file_filter(m_file_filter);
            view->when_row_dbl_click.connect(
                [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.row_data);
                });
            view->when_operation_progress.connect(
                [this](OperationProgressEvent &ctx)
                {
                    this->when_operation_progress.run(ctx);
                });

            if (m_context_menu_factory)
            {
                view->set_context_menu_factory(m_context_menu_factory);
            }
            new_view_ptr = view.get();
            add_child(std::move(view));
        }

        // Apply search
        if (!m_search_query.empty())
        {
            set_search_query(m_search_query);
        }

        if (new_view_ptr)
        {
            if (auto *child = dynamic_cast<FileListView *>(new_view_ptr)) child->set_file_filter(m_file_filter);
            else if (auto *child = dynamic_cast<FileIconView *>(new_view_ptr)) child->set_file_filter(m_file_filter);
            else if (auto *child = dynamic_cast<FileCoverFlowView *>(new_view_ptr)) child->set_file_filter(m_file_filter);
            new_view_ptr->set_focus(true);
        }
    }

    void FileView::set_show_hidden_files(bool show)
    {
        m_show_hidden_files = show;
        if (m_children.empty()) return;

        if (auto *child = dynamic_cast<FileListView *>(m_children.back().get()))
        {
            child->set_show_hidden_files(show);
            child->refresh(m_current_path, m_search_query);
        }
        else if (auto *child = dynamic_cast<FileIconView *>(m_children.back().get()))
        {
            child->set_show_hidden_files(show);
            child->refresh(m_current_path, m_search_query);
        }
        else if (auto *child = dynamic_cast<FileCoverFlowView *>(m_children.back().get()))
        {
            child->set_show_hidden_files(show);
            child->refresh(m_current_path, m_search_query);
        }
    }

    void FileView::set_file_filter(const std::vector<std::string>& patterns)
    {
        m_file_filter = patterns;
        if (m_children.empty()) return;

        if (auto *child = dynamic_cast<FileListView *>(m_children.back().get())) {
            child->set_file_filter(patterns);
            child->refresh(m_current_path, m_search_query);
        }
        else if (auto *child = dynamic_cast<FileIconView *>(m_children.back().get())) {
            child->set_file_filter(patterns);
            child->refresh(m_current_path, m_search_query);
        }
        else if (auto *child = dynamic_cast<FileCoverFlowView *>(m_children.back().get())) {
            child->set_file_filter(patterns);
            child->refresh(m_current_path, m_search_query);
        }
    }

    void FileView::navigate_to(const std::string &path, bool record_history)
    {
        if (m_current_path == path) return;
        m_current_path = path;
        if (record_history)
            m_history->push(m_current_path);

        set_view_mode(m_view_mode);

        PathChangedEvent event;
        event.path = m_current_path;
        when_path_changed.run(event);
    }

    void FileView::navigate_back()
    {
        if (m_history->can_back())
            navigate_to(m_history->back(), false);
    }

    void FileView::navigate_forward()
    {
        if (m_history->can_forward())
            navigate_to(m_history->forward(), false);
    }

    std::vector<arkutils::FileInfo> FileView::get_selection() const
    {
        if (m_children.empty()) return {};

        if (auto *child = dynamic_cast<FileListView *>(m_children.back().get()))
            return child->get_selected_items();
        else if (auto *child = dynamic_cast<FileIconView *>(m_children.back().get()))
            return child->get_selected_items();
        
        return {};
    }

    void FileView::open_selection()
    {
        auto sel = get_selection();
        if (!sel.empty()) open_item(sel[0]);
    }

    void FileView::open_item(const arkutils::FileInfo &f)
    {
        if (f.type == arkutils::FileType::Directory)
        {
            std::string target_path = f.path;
            if (application())
            {
                application()->post_task([this, target_path]()
                { this->navigate_to(target_path); });
            }
        }
        else
        {
            auto mutable_f = f;
            when_item_opened.run(mutable_f);
        }
    }

    void FileView::set_context_menu_factory(std::function<std::unique_ptr<Menu>(const arkutils::FileInfo &)> factory)
    {
        m_context_menu_factory = factory;
        if (m_children.empty()) return;

        if (auto *child = dynamic_cast<FileListView *>(m_children.back().get()))
            child->set_context_menu_factory(m_context_menu_factory);
        else if (auto *child = dynamic_cast<FileIconView *>(m_children.back().get()))
            child->set_context_menu_factory(m_context_menu_factory);
        else if (auto *child = dynamic_cast<FileCoverFlowView *>(m_children.back().get()))
            child->set_context_menu_factory(m_context_menu_factory);
    }

    void FileView::set_search_query(const std::string &query)
    {
        m_search_query = query;
        if (m_children.empty()) return;

        if (auto *child = dynamic_cast<FileListView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
        else if (auto *child = dynamic_cast<FileIconView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
        else if (auto *child = dynamic_cast<FileCoverFlowView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
    }

    bool FileView::can_back() const { return m_history->can_back(); }
    bool FileView::can_forward() const { return m_history->can_forward(); }
    const std::string &FileView::current_path() const { return m_current_path; }

    bool FileView::can_perform(ClipboardAction action) const
    {
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
            return !get_selection().empty();
        return (action == ClipboardAction::Paste);
    }

    void FileView::perform(ClipboardAction action)
    {
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
        {
            auto selection = get_selection();
            m_clipboard_paths.clear();
            for (const auto &item : selection) m_clipboard_paths.push_back(item.path);
            m_is_cut = (action == ClipboardAction::Cut);

            if (application()) application()->set_clipboard_owner(this);
        }
        else if (action == ClipboardAction::Paste)
        {
            if (application()) application()->request_clipboard_data(this, "text/uri-list");
        }
    }

    void FileView::provide_clipboard_data(const std::string &mime, DataSink &sink)
    {
        if (mime == "text/uri-list")
        {
            std::string data;
            for (const auto &path : m_clipboard_paths) data += "file://" + path + "\r\n";
            sink.write(std::vector<uint8_t>(data.begin(), data.end()));
            sink.done();
        }
        else sink.error();
    }

    void FileView::on_clipboard_data_received(const std::string &mime, const std::vector<uint8_t> &data)
    {
        if (mime != "text/uri-list" || data.empty()) return;

        std::string content(data.begin(), data.end());
        std::stringstream ss(content);
        std::string line;
        std::vector<std::string> paths;

        while (std::getline(ss, line))
        {
            if (line.empty()) continue;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.find("file://") == 0) paths.push_back(line.substr(7));
        }

        if (paths.empty()) return;

        // Note: For move/copy operations, FileView should ideally emit a signal
        // or use arkutils::FileOperations directly.
        // For now, we'll keep it simple and just do the operations.
        for (const auto &src_path : paths)
        {
            std::filesystem::path src(src_path);
            std::filesystem::path dst_dir(m_current_path);
            std::filesystem::path dst = dst_dir / src.filename();

            if (src == dst_dir || dst_dir.string().find(src.string() + "/") == 0) continue;
            if (std::filesystem::exists(dst)) continue;

            if (m_is_cut)
            {
                auto future = arkutils::FileOperations::move(src_path, dst.string());
                std::thread([this, f = std::move(future), src_path]() mutable {
                    auto result = f.get();
                    if (application())
                    {
                        application()->post_task([this, result, src_path]() {
                            if (result == arkutils::FileOperations::Result::Success)
                                this->navigate_to(m_current_path);
                        });
                    }
                }).detach();
            }
            else
            {
                auto filename = src.filename().string();
                auto future = arkutils::FileOperations::copy(src_path, m_current_path, [this](double progress) {
                    if (application()) {
                        application()->post_task([this, progress]() {
                            OperationProgressEvent ev;
                            ev.progress = progress;
                            ev.finished = (progress >= 1.0);
                            this->when_operation_progress.run(ev);
                        });
                    }
                });
                std::thread([this, f = std::move(future), filename]() mutable {
                    auto result = f.get();
                    if (application())
                    {
                        application()->post_task([this, result, filename]() {
                            if (result == arkutils::FileOperations::Result::Success) {
                                NotificationSender::send("Copia finalizada", "El archivo " + filename + " se ha copiado correctamente.", "edit-copy");
                                
                                OperationProgressEvent ev;
                                ev.progress = 1.0;
                                ev.finished = true;
                                this->when_operation_progress.run(ev);

                                this->navigate_to(m_current_path);
                            }
                        });
                    }
                }).detach();
            }
        }
    }

    std::vector<std::string> FileView::provided_mime_types() const
    {
        return {"text/uri-list"};
    }

} // namespace horizon::files
