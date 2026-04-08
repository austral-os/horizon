#include "ArkfmView.hpp"
#include "ArkfmCoverFlowView.hpp"
#include "ArkfmIconView.hpp"
#include "ArkfmListView.hpp"
#include "ArkfmWindow.hpp"
#include "NavigationHistory.hpp"
#include <horizon/Logger.hpp>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/Clipboard.hpp>
#include <horizon/ClipboardProvider.hpp>
#include <horizon/arkutils/FileOperations.hpp>
#include <sys/stat.h>
#include <filesystem>
#include <sstream>

namespace horizon::arkfm
{

    ArkfmView::ArkfmView(std::string path)
        : Widget(), m_current_path(std::move(path)),
          m_history(std::make_unique<NavigationHistory>())
    {
        m_history->push(m_current_path);
        set_view_mode(ViewMode::List);
    }

    ArkfmView::~ArkfmView() = default;

    void ArkfmView::set_view_mode(ViewMode vm)
    {
        m_view_mode = vm;
        clear_children();

        if (m_view_mode == ViewMode::List)
        {
            auto view_mode_list = std::make_unique<ArkfmListView>(m_current_path);

            view_mode_list->when_row_dbl_click.connect(
                [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.row_data);
                });

            add_child(std::move(view_mode_list));
        }
        else if (m_view_mode == ViewMode::Grid)
        {
            auto view_mode_grid = std::make_unique<ArkfmIconView>(m_current_path);

            view_mode_grid->when_item_dbl_click.connect(
                [this](horizon::IconViewItemMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.item_data);
                });

            add_child(std::move(view_mode_grid));
        }
        else if (m_view_mode == ViewMode::CoverFlow)
        {
            auto view_mode_cover = std::make_unique<ArkfmCoverFlowView>(m_current_path);

            // Handle navigation via double click if desired, but user didn't ask yet.
            // For now, just show it.

            view_mode_cover->when_row_dbl_click.connect(
                [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
                {
                    this->open_item(ctx.row_data);
                });

            add_child(std::move(view_mode_cover));
        }

        // Apply existing search query if any
        if (!m_search_query.empty())
        {
            if (auto *child = dynamic_cast<ArkfmListView *>(m_children.back().get()))
                child->refresh(m_current_path, m_search_query);
            else if (auto *child = dynamic_cast<ArkfmIconView *>(m_children.back().get()))
                child->refresh(m_current_path, m_search_query);
            else if (auto *child = dynamic_cast<ArkfmCoverFlowView *>(m_children.back().get()))
                child->refresh(m_current_path, m_search_query);
        }
    }

    void ArkfmView::navigate_to(const std::string &path, bool record_history)
    {
        m_current_path = path;
        if (record_history)
        {
            m_history->push(m_current_path);
        }

        set_view_mode(m_view_mode);

        PathChangedEvent event;
        event.path = m_current_path;
        when_path_changed.run(event);
    }

    void ArkfmView::navigate_back()
    {
        if (m_history->can_back())
        {
            navigate_to(m_history->back(), false);
        }
    }

    void ArkfmView::navigate_forward()
    {
        if (m_history->can_forward())
        {
            navigate_to(m_history->forward(), false);
        }
    }

    std::vector<arkutils::FileInfo> ArkfmView::get_selection() const
    {
        if (m_children.empty())
        {
            LOG_INFO << "ArkfmView: get_selection() - No children widgets found";
            return {};
        }

        std::vector<arkutils::FileInfo> result;
        if (auto *child = dynamic_cast<ArkfmListView *>(m_children.back().get()))
            result = child->get_selected_items();
        else if (auto *child = dynamic_cast<ArkfmIconView *>(m_children.back().get()))
            result = child->get_selected_items();
        
        LOG_INFO << "ArkfmView: get_selection() returned " << result.size() << " items";
        return result;
    }

    void ArkfmView::open_selection()
    {
        auto sel = get_selection();
        if (sel.empty())
            return;

        open_item(sel[0]);
    }

    void ArkfmView::open_item(const arkutils::FileInfo &f)
    {
        if (f.type == arkutils::FileType::Directory)
        {
            // We MUST defer navigation because this signal is emitted by the child
            // we are about to destroy in navigate_to -> set_view_mode ->
            // clear_children.
            std::string target_path = f.path;
            if (application())
            {
                application()->post_task([this, target_path]()
                                         { this->navigate_to(target_path); });
            }
        }
        else if (f.extension == "desktop")
        {
            ApplicationLauncher::launch_from_desktop_file(f.path);
        }
        else if (f.permissions & (S_IXUSR | S_IXGRP | S_IXOTH))
        {
            if (auto *win = dynamic_cast<ArkfmWindow *>(application()->root()))
            {
                if (win->confirm("¿Desea ejecutar esta aplicación?", "Confirmar ejecución"))
                {
                    ApplicationLauncher::launch_binary(f.path);
                }
            }
        }
        else
        {
            // Generic file opening via xdg-mime
            ApplicationLauncher::open_file(f.path);
        }
    }

    void ArkfmView::set_search_query(const std::string &query)
    {
        m_search_query = query;
        if (m_children.empty())
            return;

        // Refresh the active view with the new filter
        if (auto *child = dynamic_cast<ArkfmListView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
        else if (auto *child = dynamic_cast<ArkfmIconView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
        else if (auto *child = dynamic_cast<ArkfmCoverFlowView *>(m_children.back().get()))
            child->refresh(m_current_path, m_search_query);
    }

    bool ArkfmView::can_back() const
    {
        return m_history->can_back();
    }

    bool ArkfmView::can_forward() const
    {
        return m_history->can_forward();
    }

    const std::string &ArkfmView::current_path() const
    {
        return m_current_path;
    }

    bool ArkfmView::can_perform(ClipboardAction action) const
    {
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
        {
            return !get_selection().empty();
        }
        if (action == ClipboardAction::Paste)
        {
            // We can almost always attempt a paste in the current directory
            return true;
        }
        return false;
    }

    void ArkfmView::perform(ClipboardAction action)
    {
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
        {
            auto selection = get_selection();
            m_clipboard_paths.clear();
            for (const auto &item : selection)
            {
                m_clipboard_paths.push_back(item.path);
            }
            m_is_cut = (action == ClipboardAction::Cut);

            if (application())
            {
                application()->set_clipboard_owner(this);
                auto *win = dynamic_cast<ArkfmWindow *>(application()->root());
                if (win)
                {
                    win->show_status_message(m_is_cut ? "Cortado al portapapeles" : "Copiado al portapapeles");
                    LOG_INFO << "ArkfmView: perform(" << (m_is_cut ? "Cut" : "Copy") << ") - Selection copied to clipboard. Paths: " << m_clipboard_paths.size();
                }
                else
                {
                    LOG_INFO << "ArkfmView: perform() - Could not find ArkfmWindow root";
                }
            }
            else
            {
                LOG_INFO << "ArkfmView: perform() - No application() found";
            }
        }
        else if (action == ClipboardAction::Paste)
        {
            if (application())
            {
                LOG_INFO << "ArkfmView: Requesting clipboard data (text/uri-list)";
                // We request URI list specifically for file manager interop
                application()->request_clipboard_data(this, "text/uri-list");
            }
        }
    }

    void ArkfmView::provide_clipboard_data(const std::string &mime, DataSink &sink)
    {
        if (mime == "text/uri-list")
        {
            LOG_INFO << "ArkfmView: provide_clipboard_data() - Providing " << m_clipboard_paths.size() << " paths for mime " << mime;
            std::string data;
            for (const auto &path : m_clipboard_paths)
            {
                data += "file://" + path + "\r\n";
            }
            sink.write(std::vector<uint8_t>(data.begin(), data.end()));
            sink.done();
        }
        else
        {
            LOG_INFO << "ArkfmView: provide_clipboard_data() - Unsupported mime: " << mime;
            sink.error();
        }
    }

    void ArkfmView::on_clipboard_data_received(const std::string &mime, const std::vector<uint8_t> &data)
    {
        LOG_INFO << "ArkfmView: Received clipboard data. Mime: " << mime << ", Bytes: " << data.size();
        if (mime != "text/uri-list" || data.empty())
            return;

        std::string content(data.begin(), data.end());
        std::stringstream ss(content);
        std::string line;
        std::vector<std::string> paths;

        while (std::getline(ss, line))
        {
            if (line.empty())
                continue;
            // Remove \r and "file://" prefix
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.find("file://") == 0)
            {
                paths.push_back(line.substr(7));
            }
        }

        if (paths.empty())
            return;

        auto *win = dynamic_cast<ArkfmWindow *>(application()->root());
        if (!win)
            return;

        for (const auto &src_path : paths)
        {
            std::filesystem::path src(src_path);
            std::filesystem::path dst_dir(m_current_path);
            std::filesystem::path dst = dst_dir / src.filename();

            if (src == dst_dir || dst_dir.string().find(src.string() + "/") == 0)
            {
                win->alert("No es posible realizar la acción: El destino es el mismo que el origen o un subdirectorio del mismo.", "Error", MessageType::Error);
                continue;
            }

            if (std::filesystem::exists(dst))
            {
                win->alert("Ya existe un archivo o carpeta con el nombre '" + src.filename().string() + "' en el destino.", "Acción Abortada", MessageType::Warning);
                continue;
            }

            win->show_status_message(m_is_cut ? "Moviendo..." : "Copiando...");

            if (m_is_cut)
            {
                auto future = arkutils::FileOperations::move(src_path, dst.string());
                std::thread([this, win, f = std::move(future), src_path]() mutable {
                    auto result = f.get();
                    if (application())
                    {
                        application()->post_task([this, win, result, src_path]() {
                            if (result == arkutils::FileOperations::Result::Success)
                            {
                                win->show_status_message("Movido con éxito");
                                // If it was a cut from OURSELVES, clear it
                                if (std::find(m_clipboard_paths.begin(), m_clipboard_paths.end(), src_path) != m_clipboard_paths.end())
                                {
                                    m_is_cut = false;
                                }
                                this->navigate_to(m_current_path);
                            }
                            else
                            {
                                win->alert("Error al intentar mover el archivo o carpeta.", "Error", MessageType::Error);
                            }
                        });
                    }
                }).detach();
            }
            else
            {
                auto future = arkutils::FileOperations::copy(src_path, m_current_path, nullptr);
                std::thread([this, win, f = std::move(future)]() mutable {
                    auto result = f.get();
                    if (application())
                    {
                        application()->post_task([this, win, result]() {
                            if (result == arkutils::FileOperations::Result::Success)
                            {
                                win->show_status_message("Copiado con éxito");
                                this->navigate_to(m_current_path);
                            }
                            else
                            {
                                win->alert("Error al intentar copiar el archivo o carpeta.", "Error", MessageType::Error);
                            }
                        });
                    }
                }).detach();
            }
        }
    }

    std::vector<std::string> ArkfmView::provided_mime_types() const
    {
        return {"text/uri-list"};
    }

} // namespace horizon::arkfm