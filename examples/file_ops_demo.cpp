#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <iostream>

using namespace horizon;

class FileOpsDemoWindow : public Window {
public:
    FileOpsDemoWindow() : Window("Demo de Operaciones de Archivos") {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);
        set_spacing(20);

        auto label = std::make_unique<Label>("Prueba del Menú Archivo");
        label->set_font_size(24);
        label->set_font_weight(FONT_WEIGHT_BOLD);
        label->set_alignment(TextAlignment::Center);
        m_status_label = label.get();
        add_child(std::move(label));

        auto info = std::make_unique<Label>("El menú 'Archivo' debería incluir Guardar y Guardar como.");
        info->set_alignment(TextAlignment::Center);
        add_child(std::move(info));

        // Conectar los nuevos eventos automatizados
        when_file_opened.connect([this](Window::FileOpenedContext& ctx) {
            LOG_INFO << "Demo: Archivo abierto automáticamente: " << ctx.path;
            m_status_label->set_text("Abierto: " + ctx.path);
            invalidate();
        });

        when_folder_opened.connect([this](Window::FileOpenedContext& ctx) {
            LOG_INFO << "Demo: Carpeta abierta automáticamente: " << ctx.path;
            m_status_label->set_text("Carpeta: " + ctx.path);
            invalidate();
        });

        when_file_close.connect([this](EventContext&) {
            LOG_INFO << "Demo: Evento de cierre de archivo recibido!";
            m_status_label->set_text("Estado: Cerrado");
            m_current_path = "";
            invalidate();
        });

        when_save.connect([this](Window::FileSaveContext& ctx) {
            LOG_INFO << "Demo: Evento GUARDAR (Save) recibido para: " << ctx.path;
            m_status_label->set_text("Guardado: " + ctx.path);
            invalidate();
        });

        when_save_as.connect([this](Window::FileSaveContext& ctx) {
            LOG_INFO << "Demo: Evento GUARDAR COMO (Save As) recibido para: " << ctx.path;
            m_status_label->set_text("Guardado como: " + ctx.path);
            m_current_path = ctx.path; // Simular que ahora tenemos un path
            invalidate();
        });

        // También podemos seguir usando señales si lo preferimos
        signals.connect("file.opened", [this](SignalContext& ctx) {
            auto* fctx = static_cast<Window::FileOpenedContext*>(ctx.data);
            LOG_INFO << "Demo: Señal 'file.opened' recibida con ruta: " << fctx->path;
        });
    }

    uint32_t file_capabilities() const override {
        // Habilitamos todas las capacidades
        return FileAll;
    }

    std::string current_file_path() const override {
        return m_current_path;
    }

private:
    Label *m_status_label{nullptr};
    std::string m_current_path;
};

int main(int argc, char** argv) {
    try {
        Application app("horizon.file_demo", 500, 300);
        app.set_name("File Operations Demo");

        app.set_root(std::make_unique<FileOpsDemoWindow>());
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
