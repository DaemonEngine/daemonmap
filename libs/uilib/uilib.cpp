#include "uilib.h"

#include <tuple>

#include <gtk/gtk.h>

#include "gtkutil/dialog.h"
#include "gtkutil/filechooser.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/window.h"

namespace ui {

    void init(int argc, char *argv[])
    {
        gtk_disable_setlocale();
        gtk_init(&argc, &argv);
    }

    void main()
    {
        gtk_main();
    }

    Widget root{nullptr};

#define this (*static_cast<self>(this))

    void IEditable::editable(bool value)
    {
        gtk_editable_set_editable(GTK_EDITABLE(this), value);
    }

    Widget::Widget() : Widget(nullptr)
    {}

    alert_response IWidget::alert(std::string text, std::string title, alert_type type, alert_icon icon)
    {
        auto ret = gtk_MessageBox(this, text.c_str(),
                                  title.c_str(),
                                  type == alert_type::OK ? eMB_OK :
                                  type == alert_type::OKCANCEL ? eMB_OKCANCEL :
                                  type == alert_type::YESNO ? eMB_YESNO :
                                  type == alert_type::YESNOCANCEL ? eMB_YESNOCANCEL :
                                  type == alert_type::NOYES ? eMB_NOYES :
                                  eMB_OK,
                                  icon == alert_icon::Default ? eMB_ICONDEFAULT :
                                  icon == alert_icon::Error ? eMB_ICONERROR :
                                  icon == alert_icon::Warning ? eMB_ICONWARNING :
                                  icon == alert_icon::Question ? eMB_ICONQUESTION :
                                  icon == alert_icon::Asterisk ? eMB_ICONASTERISK :
                                  eMB_ICONDEFAULT
        );
        return
                ret == eIDOK ? alert_response::OK :
                ret == eIDCANCEL ? alert_response::CANCEL :
                ret == eIDYES ? alert_response::YES :
                ret == eIDNO ? alert_response::NO :
                alert_response::OK;
    }

    const char *
    IWidget::file_dialog(bool open, const char *title, const char *path, const char *pattern, bool want_load,
                         bool want_import, bool want_save)
    {
        return ::file_dialog(this, open, title, path, pattern, want_load, want_import, want_save);
    }

    Window::Window() : Window(nullptr)
    {}

    Window::Window(window_type type) : Window(GTK_WINDOW(gtk_window_new(
            type == window_type::TOP ? GTK_WINDOW_TOPLEVEL :
            type == window_type::POPUP ? GTK_WINDOW_POPUP :
            GTK_WINDOW_TOPLEVEL
    )))
    {}

    Window IWindow::create_dialog_window(const char *title, void func(), void *data, int default_w, int default_h)
    {
        return Window(::create_dialog_window(this, title, func, data, default_w, default_h));
    }

    Window IWindow::create_modal_dialog_window(const char *title, ModalDialog &dialog, int default_w, int default_h)
    {
        return Window(::create_modal_dialog_window(this, title, dialog, default_w, default_h));
    }

    Window IWindow::create_floating_window(const char *title)
    {
        return Window(::create_floating_window(title, this));
    }

    std::uint64_t IWindow::on_key_press(bool (*f)(Widget widget, _GdkEventKey *event, void *extra), void *extra)
    {
        using f_t = decltype(f);
        struct user_data {
            f_t f;
            void *extra;
        } *pass = new user_data{f, extra};
        auto dtor = [](user_data *data, GClosure *) {
            delete data;
        };
        auto func = [](_GtkWidget *widget, GdkEventKey *event, user_data *args) -> bool {
            return args->f(Widget(widget), event, args->extra);
        };
        auto clos = g_cclosure_new(G_CALLBACK(+func), pass, reinterpret_cast<GClosureNotify>(+dtor));
        return g_signal_connect_closure(G_OBJECT(this), "key-press-event", clos, false);
    }

    void IWindow::add_accel_group(AccelGroup group)
    {
        gtk_window_add_accel_group(this, group);
    }

    Alignment::Alignment(float xalign, float yalign, float xscale, float yscale)
            : Alignment(GTK_ALIGNMENT(gtk_alignment_new(xalign, yalign, xscale, yscale)))
    {}

    Frame::Frame(const char *label) : Frame(GTK_FRAME(gtk_frame_new(label)))
    {}

    Button::Button() : Button(GTK_BUTTON(gtk_button_new()))
    {}

    Button::Button(const char *label) : Button(GTK_BUTTON(gtk_button_new_with_label(label)))
    {}

    bool IToggleButton::active()
    {
        return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(this)) != 0;
    }

    CheckButton::CheckButton(const char *label) : CheckButton(GTK_CHECK_BUTTON(gtk_check_button_new_with_label(label)))
    {}

    MenuItem::MenuItem() : MenuItem(GTK_MENU_ITEM(gtk_menu_item_new()))
    {}

    MenuItem::MenuItem(const char *label, bool mnemonic) : MenuItem(
            GTK_MENU_ITEM((mnemonic ? gtk_menu_item_new_with_mnemonic : gtk_menu_item_new_with_label)(label)))
    {}

    TearoffMenuItem::TearoffMenuItem() : TearoffMenuItem(GTK_TEAROFF_MENU_ITEM(gtk_tearoff_menu_item_new()))
    {}

    ComboBoxText::ComboBoxText() : ComboBoxText(GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new()))
    {}

    ScrolledWindow::ScrolledWindow() : ScrolledWindow(GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(nullptr, nullptr)))
    {}

    VBox::VBox(bool homogenous, int spacing) : VBox(GTK_VBOX(gtk_vbox_new(homogenous, spacing)))
    {}

    HBox::HBox(bool homogenous, int spacing) : HBox(GTK_HBOX(gtk_hbox_new(homogenous, spacing)))
    {}

    HPaned::HPaned() : HPaned(GTK_HPANED(gtk_hpaned_new()))
    {}

    VPaned::VPaned() : VPaned(GTK_VPANED(gtk_vpaned_new()))
    {}

    Menu::Menu() : Menu(GTK_MENU(gtk_menu_new()))
    {}

    Table::Table(std::size_t rows, std::size_t columns, bool homogenous) : Table(
            GTK_TABLE(gtk_table_new(rows, columns, homogenous))
    )
    {}

    TextView::TextView() : TextView(GTK_TEXT_VIEW(gtk_text_view_new()))
    {}

    TreeView::TreeView() : TreeView(GTK_TREE_VIEW(gtk_tree_view_new()))
    {}

    TreeView::TreeView(TreeModel model) : TreeView(GTK_TREE_VIEW(gtk_tree_view_new_with_model(model)))
    {}

    Label::Label(const char *label) : Label(GTK_LABEL(gtk_label_new(label)))
    {}

    Image::Image() : Image(GTK_IMAGE(gtk_image_new()))
    {}

    Entry::Entry() : Entry(GTK_ENTRY(gtk_entry_new()))
    {}

    Entry::Entry(std::size_t max_length) : Entry()
    {
        gtk_entry_set_max_length(this, static_cast<gint>(max_length));
    }

    SpinButton::SpinButton(Adjustment adjustment, double climb_rate, std::size_t digits) : SpinButton(
            GTK_SPIN_BUTTON(gtk_spin_button_new(adjustment, climb_rate, digits)))
    {}

    HScale::HScale(Adjustment adjustment) : HScale(GTK_HSCALE(gtk_hscale_new(adjustment)))
    {}

    HScale::HScale(double min, double max, double step) : HScale(GTK_HSCALE(gtk_hscale_new_with_range(min, max, step)))
    {}

    Adjustment::Adjustment(double value,
                           double lower, double upper,
                           double step_increment, double page_increment,
                           double page_size)
            : Adjustment(
            GTK_ADJUSTMENT(gtk_adjustment_new(value, lower, upper, step_increment, page_increment, page_size)))
    {}

    CellRendererText::CellRendererText() : CellRendererText(GTK_CELL_RENDERER_TEXT(gtk_cell_renderer_text_new()))
    {}

    TreeViewColumn::TreeViewColumn(const char *title, CellRenderer renderer,
                                   std::initializer_list<TreeViewColumnAttribute> attributes)
            : TreeViewColumn(gtk_tree_view_column_new_with_attributes(title, renderer, nullptr))
    {
        for (auto &it : attributes) {
            gtk_tree_view_column_add_attribute(this, renderer, it.attribute, it.column);
        }
    };

    AccelGroup::AccelGroup() : AccelGroup(GTK_ACCEL_GROUP(gtk_accel_group_new()))
    {}

    TreePath::TreePath() : TreePath(gtk_tree_path_new())
    {}

    TreePath::TreePath(const char *path) : TreePath(gtk_tree_path_new_from_string(path))
    {}

}
