#include "ArkfmWindow.hpp"
#include <horizon/files/FileSidebar.hpp>
#include <horizon/files/FileToolbar.hpp>
#include <horizon/files/FileView.hpp>
#include "dialogs/NewFolderDialog.hpp"
#include "dialogs/RenameDialog.hpp"
#include "dialogs/PropertiesDialog.hpp"
#include "dialogs/GoToFolderDialog.hpp"
#include "dialogs/ConnectToServerDialog.hpp"
#include <horizon/storage/MountPasswordDialog.hpp>
#include <horizon/storage/RemoteManager.hpp>
#include <horizon/I18n.hpp>
#include "horizon/ApplicationWindow.hpp"
#include <horizon/DialogTypes.hpp>
#include "horizon/Label.hpp"
#include "horizon/Logger.hpp"
#include "horizon/ProgressBar.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/VPanel.hpp"
#include "horizon/Widget.hpp"
#include "horizon/arkutils/FileOperations.hpp"
#include "horizon/ApplicationLauncher.hpp"
#include "horizon/Menu.hpp"
#include <algorithm>
#include <filesystem>
#include <memory>
#include <thread>

namespace horizon::arkfm
{

    ArkfmWindow::ArkfmWindow(int w, int h) : ApplicationWindow("Ark File Manager")
    {
        set_size(w, h);
        auto ark_toolbar = std::make_unique<files::FileToolbar>();
        auto *ark_toolbar_ptr = ark_toolbar.get();
        toolbar()->add_toolbar_widget(std::move(ark_toolbar));
        show_status_bar();

        auto vpanel = std::make_unique<horizon::VPanel>();
        vpanel->set_spacing(10);

        auto sidebar = std::make_unique<files::FileSidebar>();
        m_sidebar_ptr = sidebar.get();
        auto *sidebar_ptr = m_sidebar_ptr;
        auto view = std::make_unique<files::FileView>(getenv("HOME") ? getenv("HOME") : "~/");
        m_view_ptr = view.get();
        auto *view_ptr = m_view_ptr;

        sidebar_ptr->when_item_selected.connect(
            [view_ptr](horizon::SidebarItemSelectedContext &ctx)
            {
                if (!ctx.item->path().empty())
                {
                    view_ptr->navigate_to(ctx.item->path());
                }
            });

        // Initialize Remote Storage from the new library
        m_remote_manager = std::make_unique<storage::RemoteManager>();
        m_sidebar_ptr->set_remote_storage(m_remote_manager.get());

        std::string home_path = getenv("HOME") ? getenv("HOME") : "/";
        m_sidebar_ptr->select_item_by_path(home_path);

        m_sidebar_ptr->when_resource_unmounted.connect([this](horizon::files::UnmountEventContext& ctx) {
            if (m_view_ptr) {
                std::string current_path = m_view_ptr->current_path();
                if (!ctx.mount_path.empty() && current_path.find(ctx.mount_path) == 0) {
                    std::string home = getenv("HOME") ? getenv("HOME") : "/";
                    LOG_INFO << "ArkfmWindow: El recurso en " << ctx.mount_path << " ha sido desmontado. Redirigiendo de " << current_path << " a " << home;
                    m_view_ptr->navigate_to(home);
                }
            }
        });

        show_status_bar();
        auto *sb = statusbar();
        auto lbl = std::make_unique<horizon::Label>("");
        m_status_label = lbl.get();

        auto pbc = std::make_unique<horizon::Widget>();
        auto pb = std::make_unique<horizon::ProgressBar>();

        m_progress_bar = pb.get();
        m_progress_bar->set_visible(false);
        m_progress_bar->set_fixed_size(10);

        pbc->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        pbc->set_fixed_size(200);

        pbc->add_child(horizon::Spacer());
        pbc->add_child(std::move(pb));
        pbc->add_child(horizon::Spacer());

        sb->add_child(horizon::Spacer(10));
        sb->add_child(std::move(lbl));
        sb->add_child(std::move(pbc));
        sb->add_child(horizon::Spacer(10));

        ark_toolbar_ptr->when_navigation_clicked.connect(
            [view_ptr](files::NavigationButtonClickEvent &ctx)
            {
                if (ctx.index == 0)
                {
                    view_ptr->navigate_back();
                }
                else
                {
                    view_ptr->navigate_forward();
                }
            });

        ark_toolbar_ptr->when_view_mode_changed.connect(
            [view_ptr](files::ViewModeChangeEvent &ctx)
            {
                if (ctx.view_mode_index == 0)
                {
                    view_ptr->set_view_mode(files::ViewMode::Grid);
                }
                else if (ctx.view_mode_index == 1)
                {
                    view_ptr->set_view_mode(files::ViewMode::List);
                }
                else if (ctx.view_mode_index == 3)
                {
                    view_ptr->set_view_mode(files::ViewMode::CoverFlow);
                }
            });

        ark_toolbar_ptr->when_search_changed.connect(
            [view_ptr](files::SearchChangedEvent &ctx)
            {
                view_ptr->set_search_query(ctx.query);
            });

        m_view_ptr->when_item_opened.connect(
            [](const arkutils::FileInfo &f)
            {
                if (f.type == arkutils::FileType::Regular)
                {
                    ApplicationLauncher::open_file(f.path);
                }
            });

        m_view_ptr->set_context_menu_factory([this](const arkutils::FileInfo &f) {
            auto menu = std::make_unique<horizon::Menu>();
            
            auto item_open = menu->add_item("Abrir");
            item_open->when_click.connect([this, f](auto&) { this->m_view_ptr->open_item(f); });
            
            menu->add_separator();
            
            auto item_rename = menu->add_item("Renombrar");
            item_rename->when_click.connect([this, f](auto&) { this->handle_rename(f.path); });
            
            auto item_delete = menu->add_item("Eliminar");
            item_delete->when_click.connect([this, f](auto&) { this->handle_delete(f.path); });
            
            menu->add_separator();
            
            auto item_props = menu->add_item("Propiedades");
            item_props->when_click.connect([this, f](auto&) { 
                auto dialog = std::make_unique<PropertiesDialog>(f);
                dialog->run();
            });

            menu->add_separator();
            auto item_connect = menu->add_item("Conectar al servidor...");
            item_connect->when_click.connect([this](auto&) {
                application()->signal_manager.emit("go-connect");
            });
            
            return menu;
        });

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(view));
        set_content(std::move(vpanel));

        this->when_application_load.connect(
            [this, view_ptr](EventContext &)
            {
                if (application())
                {
                    application()->signal_manager.connect(
                        "new-folder", [this](SignalContext &)
                        { this->handle_new_folder(); });

                    application()->signal_manager.connect(
                        "delete", [this](SignalContext &)
                        {
                            auto sel = m_view_ptr->get_selection();
                            if (!sel.empty())
                                handle_delete(sel[0].path);
                        });
                    application()->signal_manager.connect(
                        "properties", [this](SignalContext &)
                        { handle_properties(); });

                    // Go Menu Handlers
                    application()->signal_manager.connect("go-back", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_back(); 
                    });
                    application()->signal_manager.connect("go-forward", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_forward(); 
                    });
                    application()->signal_manager.connect("go-parent", [this](SignalContext&) {
                        if (m_view_ptr) {
                            std::filesystem::path p(m_view_ptr->current_path());
                            m_view_ptr->navigate_to(p.parent_path().string());
                        }
                    });
                    application()->signal_manager.connect("go-home", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_to(getenv("HOME") ? getenv("HOME") : "/"); 
                    });
                    application()->signal_manager.connect("go-documents", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_to(std::string(getenv("HOME") ? getenv("HOME") : "/") + "/Documents"); 
                    });
                    application()->signal_manager.connect("go-desktop", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_to(std::string(getenv("HOME") ? getenv("HOME") : "/") + "/Desktop"); 
                    });
                    application()->signal_manager.connect("go-downloads", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_to(std::string(getenv("HOME") ? getenv("HOME") : "/") + "/Downloads"); 
                    });
                    application()->signal_manager.connect("go-videos", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_to(std::string(getenv("HOME") ? getenv("HOME") : "/") + "/Videos"); 
                    });
                    application()->signal_manager.connect("go-images", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_to(std::string(getenv("HOME") ? getenv("HOME") : "/") + "/Images"); 
                    });
                    application()->signal_manager.connect("go-applications", [this](SignalContext&) { 
                        if (m_view_ptr) m_view_ptr->navigate_to("/usr/share/applications"); 
                    });
                    application()->signal_manager.connect("go-to-folder", [this](SignalContext&) {
                        application()->post_task([this]() {
                            application()->set_override_cursor(CursorType::Wait);
                            auto dialog = std::make_unique<GoToFolderDialog>();
                            dialog->when_accepted.connect([this](GoToFolderEvent& ev) {
                                if (m_view_ptr) m_view_ptr->navigate_to(ev.path);
                            });
                            dialog->run();
                            application()->clear_override_cursor();
                        });
                    });

                    application()->signal_manager.connect("go-connect", [this](SignalContext&) {
                        application()->post_task([this]() {
                            application()->set_override_cursor(CursorType::Wait);
                            auto dialog = std::make_unique<ConnectToServerDialog>();
                            dialog->when_accepted.connect([this](ConnectToServerEvent& ev) {
                                this->handle_mount_remote(ev.uri);
                            });
                            dialog->run();
                            application()->clear_override_cursor();
                        });
                    });

                    m_view_ptr->when_right_click.connect(
                        [this](MouseButtonEventContext &ctx)
                        {
                            if (!ctx.stop_propagation)
                            {
                                m_active_context_menu = std::make_unique<horizon::Menu>();
                                auto item_new = m_active_context_menu->add_item("Nueva carpeta");
                                item_new->when_click.connect([this](auto &)
                                                               { this->handle_new_folder(); });

                                m_active_context_menu->add_separator();

                                auto item_props = m_active_context_menu->add_item("Propiedades");
                                item_props->when_click.connect([this](auto &)
                                                               { this->handle_properties(); });

                                m_active_context_menu->add_separator();

                                auto item_connect = m_active_context_menu->add_item("Conectar al servidor...");
                                item_connect->when_click.connect([this](auto &) {
                                    application()->signal_manager.emit("go-connect");
                                });

                                application()->show_context_menu(m_active_context_menu.get(), -1, -1, ctx.serial,
                                                                 this->m_view_ptr);
                                ctx.stop_propagation = true;
                            }
                        });

                    application()->when_popup_dismissed.connect([this](PopupDismissedContext &) {
                        m_active_context_menu.reset();
                    });
                }
            });
    }
    
    ArkfmWindow::~ArkfmWindow() = default;


    void ArkfmWindow::handle_new_folder()
    {
        if (!m_view_ptr)
            return;

        application()->post_task([this]() {
            application()->set_override_cursor(CursorType::Wait);
            auto dialog = std::make_unique<NewFolderDialog>();
            dialog->when_accepted.connect(
            [this](NewFolderEvent &ctx)
            {
                std::string full_path = m_view_ptr->current_path() + "/" + ctx.folder_name;
                show_status_message("Creando carpeta...");
                auto future = arkutils::FileOperations::create_directory(full_path);
                std::thread([this, f = std::move(future)]() mutable {
                    auto result = f.get();
                    if (application())
                    {
                        application()->post_task([this, result]() {
                            if (result == arkutils::FileOperations::Result::Success)
                            {
                                show_status_message("Carpeta creada");
                                if (m_view_ptr)
                                    m_view_ptr->navigate_to(m_view_ptr->current_path());
                            }
                            else
                            {
                                application()->alert("No se pudo crear la carpeta.", "Error",
                                                   MessageType::Error);
                            }
                        });
                    }
                }).detach();
            });
            dialog->run();
            application()->clear_override_cursor();
        });
    }

    void ArkfmWindow::handle_rename(const std::string &path)
    {
        application()->set_override_cursor(CursorType::Wait);
        std::filesystem::path p(path);
        auto dialog = std::make_unique<RenameDialog>(p.filename().string());
        dialog->when_accepted.connect([this, path, p](RenameEvent &ctx) {
            std::filesystem::path new_path = p.parent_path() / ctx.new_name;

            if (std::filesystem::exists(new_path))
            {
                application()->alert("Ya existe un archivo o carpeta con el nombre '" + ctx.new_name + "' en esta ubicación.", "Error al renombrar", MessageType::Error);
                return;
            }

            auto future = arkutils::FileOperations::rename(path, new_path.string());
            std::thread([this, f = std::move(future)]() mutable {
                auto result = f.get();
                if (application())
                {
                    application()->post_task([this, result]() {
                        if (result == arkutils::FileOperations::Result::Success)
                        {
                            show_status_message("Renombrado con éxito");
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(m_view_ptr->current_path());
                        }
                        else
                        {
                            application()->alert("No se pudo renombrar el archivo o carpeta.", "Error", MessageType::Error);
                        }
                    });
                }
            }).detach();
        });
        dialog->run();
        application()->clear_override_cursor();
    }

    void ArkfmWindow::handle_delete(const std::string &path)
    {
        application()->set_override_cursor(CursorType::Wait);
        std::string filename = std::filesystem::path(path).filename().string();
        if (application()->confirm("¿Está seguro que desea eliminar '" + filename + "'?", "Confirmar eliminación"))
        {
            show_status_message("Eliminando...");
            auto future = arkutils::FileOperations::remove(path);
            std::thread([this, f = std::move(future)]() mutable {
                auto result = f.get();
                if (application())
                {
                    application()->post_task([this, result]() {
                        if (result == arkutils::FileOperations::Result::Success)
                        {
                            show_status_message("Eliminado con éxito");
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(m_view_ptr->current_path());
                        }
                        else
                        {
                            application()->alert("Error al intentar eliminar el archivo o carpeta.", "Error", MessageType::Error);
                        }
                    });
                }
            }).detach();
        }
        application()->clear_override_cursor();
    }

    void ArkfmWindow::handle_open()
    {
        if (m_view_ptr)
            m_view_ptr->open_selection();
    }

    void ArkfmWindow::handle_properties()
    {
        if (!m_view_ptr)
            return;

        auto sel = m_view_ptr->get_selection();
        arkutils::FileInfo f;
        if (sel.empty())
        {
            f.name = std::filesystem::path(m_view_ptr->current_path()).filename().string();
            if (f.name.empty())
                f.name = "/";
            f.path = m_view_ptr->current_path();
            f.type = arkutils::FileType::Directory;
        }
        else
        {
            f = sel[0];
        }

        application()->post_task([this, f]() {
            application()->set_override_cursor(CursorType::Wait);
            auto dialog = std::make_unique<PropertiesDialog>(f);
            dialog->run();
            application()->clear_override_cursor();
        });
    }



    void ArkfmWindow::show_status_message(const std::string &msg, int timeout_ms)
    {
        if (!m_status_label)
            return;
        m_status_label->set_text(msg);

        if (application() && timeout_ms > 0)
        {
            application()->add_timer(timeout_ms,
                                     [this, msg]()
                                     {
                                         if (m_status_label && m_status_label->text() == msg)
                                         {
                                             m_status_label->set_text("");
                                         }
                                     });
        }
    }

    void ArkfmWindow::handle_mount_remote(const std::string &uri, storage::RemoteCredentials creds, storage::MountPasswordDialog* dlg)
    {
        application()->set_override_cursor(CursorType::Wait);
        LOG_INFO << "ArkFM: Intentando montar " << uri;
        if (!m_remote_manager) {
            m_remote_manager = std::make_unique<storage::RemoteManager>();
            m_sidebar_ptr->set_remote_storage(m_remote_manager.get());
        }

        // Check if already mounted
        auto active_mounts = m_remote_manager->get_active_mounts();

        // Check if we have saved credentials for this URI
        if (creds.username.empty() && !creds.is_guest) {
            storage::RemoteCredentials saved;
            if (m_remote_manager->get_credentials(uri, saved)) {
                LOG_INFO << "ArkfmWindow: Utilizando credenciales guardadas para " << uri;
                creds = saved;
            }
        }

        for (const auto& mount : active_mounts) {
            std::string n_uri = uri;
            if (!n_uri.empty() && n_uri.back() == '/') n_uri.pop_back();
            std::string m_uri = mount.uri;
            if (!m_uri.empty() && m_uri.back() == '/') m_uri.pop_back();

            if (n_uri == m_uri) {
                LOG_INFO << "ArkfmWindow: URI ya montada en " << mount.mount_path;
                application()->clear_override_cursor();
                if (!mount.mount_path.empty()) {
                    if (m_view_ptr) m_view_ptr->navigate_to(mount.mount_path);
                    if (m_sidebar_ptr) m_sidebar_ptr->select_item_by_path(mount.mount_path);
                } else {
                    LOG_WARNING << "ArkfmWindow: URI montada pero sin ruta FUSE.";
                }
                
                if (application()) {
                    application()->alert(i18n().tr("arkfm.messages.already_mounted"));
                }
                return;
            }
        }

        // Capturamos un weak_ptr para saber si el diálogo sigue vivo de forma segura
        std::weak_ptr<storage::MountPasswordDialog> weak_dlg = m_mount_dialog;

        m_remote_manager->when_mount(uri, creds, [this, uri, weak_dlg, creds](storage::RemoteMountResult res) {
            auto shared_dlg = weak_dlg.lock();
            
            // ¡ESTO ES LO IMPORTANTE!
            // Si hay diálogo, enviamos la tarea a SU cola. Si no, a la de la App.
            auto task_launcher = [this, shared_dlg](std::function<void()> task) {
                if (shared_dlg) shared_dlg->post_task(task);
                else application()->post_task(task);
            };

            task_launcher([this, uri, res, shared_dlg, creds]() {
                application()->clear_override_cursor();
                if (res.success) {
                    LOG_INFO << "ArkFM: Montado exitoso de " << uri << ". Cerrando diálogo...";
                    if (shared_dlg) {
                        shared_dlg->quit();
                        shared_dlg->wakeup();
                    }
                    // NO reseteamos aquí para evitar destruir el objeto mientras su loop corre

                    // IMPORTANTE: La navegación la hace la ventana principal, no el diálogo.
                    application()->post_task([this, res, uri, creds]() {
                        if (!res.mount_path.empty()) {
                            // If successfully mounted and 'remember' was checked, save credentials
                            if (creds.remember) {
                                m_remote_manager->save_credentials(uri, creds);
                            }

                            if (m_view_ptr) m_view_ptr->navigate_to(res.mount_path);
                            if (m_sidebar_ptr) {
                                application()->add_timer(1000, [this, path = res.mount_path]() {
                                    if (m_sidebar_ptr) {
                                        m_sidebar_ptr->refresh_devices();
                                        m_sidebar_ptr->select_item_by_path(path);
                                    }
                                }, false);
                            }
                            show_status_message("Conectado a " + res.mount_path);
                        } else {
                            LOG_WARNING << "ArkfmWindow: Montaje exitoso pero sin ruta FUSE para " << uri;
                            application()->alert("Conectado con éxito, pero no se encontró un punto de montaje local. Verifique que la ubicación sea un recurso compartido válido.", "Aviso");
                        }
                    });
                } else {
                    LOG_ERROR << "ArkFM: Fallo al montar " << uri << ": " << res.message;
                    
                    std::string msg = res.message;
                    std::string raw_msg = msg;
                    std::transform(msg.begin(), msg.end(), msg.begin(), ::tolower);

                    bool needs_auth = (msg.find("not authorized") != std::string::npos || 
                                       msg.find("password") != std::string::npos ||
                                       msg.find("contrase") != std::string::npos ||
                                       msg.find("access denied") != std::string::npos ||
                                       msg.find("permission denied") != std::string::npos ||
                                       msg.find("authentication") != std::string::npos ||
                                       msg.find("autentica") != std::string::npos ||
                                       msg.find("soportada") != std::string::npos || 
                                       msg.find("supported") != std::string::npos ||
                                       msg.find("handshake") != std::string::npos ||
                                       msg.find("refused") != std::string::npos);

                    if (needs_auth) {
                        if (shared_dlg) {
                            LOG_INFO << "ArkFM: Informando error al diálogo existente.";
                            
                            std::string display_msg = raw_msg;
                            // Si el error es "cancelado" o "no soportada", usamos nuestro mensaje genérico amigable
                            if (msg.find("cancel") != std::string::npos || 
                                msg.find("soportada") != std::string::npos || 
                                msg.find("supported") != std::string::npos) 
                            {
                                display_msg = horizon::i18n().tr("core.storage.mount_dialog.auth_error");
                            }
                            
                            shared_dlg->show_error(display_msg);
                        } else {
                            LOG_INFO << "ArkFM: Creando nuevo diálogo de contraseña.";
                            m_mount_dialog = std::make_shared<storage::MountPasswordDialog>(uri);
                            m_mount_dialog->set_initial_credentials(creds);
                            
                            // Capturamos un weak_ptr para evitar una referencia circular
                            std::weak_ptr<storage::MountPasswordDialog> weak_dlg_signal = m_mount_dialog;
                            
                            m_mount_dialog->when_accepted.connect([this, uri, weak_dlg_signal](storage::MountPasswordEvent& ev) {
                                if (auto shared_dlg_signal = weak_dlg_signal.lock()) {
                                    this->handle_mount_remote(uri, ev.credentials, shared_dlg_signal.get());
                                }
                            });
                            
                            m_mount_dialog->run();
                            m_mount_dialog.reset();
                            LOG_INFO << "ArkFM: El bucle del diálogo ha finalizado.";
                        }
                    } else {
                        std::string display_msg = raw_msg;
                        if (msg.find("cancel") != std::string::npos || 
                            msg.find("soportada") != std::string::npos || 
                            msg.find("supported") != std::string::npos) 
                        {
                            display_msg = horizon::i18n().tr("core.storage.mount_dialog.auth_error");
                        }
                        
                        if (shared_dlg) shared_dlg->show_error(display_msg);
                        show_status_message("Error: " + display_msg, 5000);
                        
                        // Show alert for real errors (excluding cancellations by user)
                        if (msg.find("cancel") == std::string::npos && application()) {
                            application()->alert(display_msg, i18n().tr("arkfm.dialog.error") != "arkfm.dialog.error" ? i18n().tr("arkfm.dialog.error") : "Error");
                        }
                    }
                }
            });
            
            if (shared_dlg) shared_dlg->wakeup();
        });
    }

} // namespace horizon::arkfm