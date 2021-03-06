//
// Created by jannis on 25.07.18.
//

#include <glibmm/i18n.h>

#include <iostream>
#include "launcher_window.hpp"
#include "../project/shader_pack_project.hpp"
#include "../application/application.hpp"

namespace shader_editor {
    launcher_window::launcher_window() : glade_window("/com/continuum/shaderpackeditor/ui/launcher_ui.glade")
#ifndef G_OS_UNIX
                                         ,new_project_dialog(_("New project"), true)
#endif
    {
        setup();
    }

    void launcher_window::setup() {
        get_widget("window", window);
        window->set_icon(Gdk::Pixbuf::create_from_resource("/com/continuum/shaderpackeditor/global/icon.png"));
        auto *new_project_ok = get_widget<Gtk::Button>("new_project_ok");

#ifdef G_OS_UNIX
        get_widget<Gtk::Button>("new_project")->signal_clicked().connect(
                sigc::mem_fun(*get_widget<Gtk::Popover>("new_project_popover"), &Gtk::Popover::show));

        new_project_ok->signal_clicked().connect(sigc::mem_fun(this, &launcher_window::new_project_ok_clicked));
#else
        // Workaround for Windows because the popover does not show...
        auto popover_content = get_widget<Gtk::Grid>("popover_content");
        auto new_project_popover = get_widget<Gtk::Popover>("new_project_popover");
        if(popover_content->get_parent() == new_project_popover) {
            new_project_popover->Container::remove(*popover_content);
        }
        new_project_dialog.set_transient_for(*window);
        new_project_dialog.get_content_area()->add(*popover_content);
        get_widget<Gtk::Button>("new_project")->signal_clicked().connect([this] {
            new_project_dialog.show();
        });

        new_project_ok->signal_clicked().connect([this]{
           new_project_dialog.close();
           new_project_ok_clicked();
        });
#endif
        get_widget<Gtk::Button>("open_project")->signal_clicked().connect(
                sigc::mem_fun(*this, &launcher_window::open_clicked));

        auto *new_project_name = get_widget<Gtk::Entry>("new_project_name");
        auto *new_project_path = get_widget<Gtk::FileChooserButton>("new_project_path");

        new_project_name->signal_changed().connect([this, new_project_name, new_project_ok]{
            name_ok = new_project_name->get_text().length() > 0;
            new_project_ok->set_sensitive(name_ok && path_ok);
        });

        new_project_path->signal_file_set().connect([this, new_project_path, new_project_ok]{
            path_ok = true;
            new_project_ok->set_sensitive(name_ok && path_ok);
        });
    }

    Gtk::Window *launcher_window::get_window() {
        return window.get();
    }

    void launcher_window::open_clicked() {
        auto *chooser = gtk_file_chooser_native_new(_("Open project"), &window->gobj()->window, GTK_FILE_CHOOSER_ACTION_OPEN, _("_Open"), _("_Cancel"));
        auto filter = Gtk::FileFilter::create();
        filter->add_pattern("*.json");
        filter->set_name("Json");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter->gobj());

        if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(chooser)) == Gtk::ResponseType::RESPONSE_ACCEPT) {
            auto result = shader_pack_project::from_file(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser)));
            if(result) {
                window->hide();
                application::instance->set_current_project(result.obj);
            } else {
                Gtk::MessageDialog err_dialog(_("Failed to open project"), false, Gtk::MessageType::MESSAGE_ERROR, Gtk::ButtonsType::BUTTONS_OK, true);
                err_dialog.set_secondary_text(result.err_message);
                err_dialog.run();
            }
        }
    }

    void launcher_window::new_project_ok_clicked() {
        auto project = shader_pack_project::from_scratch();
        project->name = get_widget<Gtk::Entry>("new_project_name")->get_text();
        project->root = get_widget<Gtk::FileChooserButton>("new_project_path")->get_filename();
        application::instance->set_current_project(project);
        window->close();
    }

}