#ifndef INCLUDED_UILIB_H
#define INCLUDED_UILIB_H

#include <string>
#include <glib-object.h>

struct _GdkEventKey;
struct _GtkAccelGroup;
struct _GtkAdjustment;
struct _GtkAlignment;
struct _GtkBin;
struct _GtkBox;
struct _GtkButton;
struct _GtkCellEditable;
struct _GtkCellRenderer;
struct _GtkCellRendererText;
struct _GtkCheckButton;
struct _GtkCheckMenuItem;
struct _GtkComboBox;
struct _GtkComboBoxText;
struct _GtkContainer;
struct _GtkDialog;
struct _GtkEditable;
struct _GtkEntry;
struct _GtkEntryCompletion;
struct _GtkFrame;
struct _GtkHBox;
struct _GtkHPaned;
struct _GtkHScale;
struct _GtkImage;
struct _GtkItem;
struct _GtkLabel;
struct _GtkListStore;
struct _GtkMenu;
struct _GtkMenuBar;
struct _GtkMenuItem;
struct _GtkMenuShell;
struct _GtkMisc;
struct _GtkObject;
struct _GtkPaned;
struct _GtkRadioButton;
struct _GtkRadioMenuItem;
struct _GtkRadioToolButton;
struct _GtkRange;
struct _GtkScale;
struct _GtkScrolledWindow;
struct _GtkSpinButton;
struct _GtkTable;
struct _GtkTearoffMenuItem;
struct _GtkTextView;
struct _GtkToggleButton;
struct _GtkToggleToolButton;
struct _GtkToolbar;
struct _GtkToolButton;
struct _GtkToolItem;
struct _GtkTreeModel;
struct _GtkTreePath;
struct _GtkTreeSelection;
struct _GtkTreeView;
struct _GtkTreeViewColumn;
struct _GtkVBox;
struct _GtkVPaned;
struct _GtkWidget;
struct _GtkWindow;
struct _GTypeInstance;

struct ModalDialog;

namespace ui {

    void init(int argc, char *argv[]);

    void main();

    void process();

    extern class Widget root;

    enum class alert_type {
        OK,
        OKCANCEL,
        YESNO,
        YESNOCANCEL,
        NOYES,
    };

    enum class alert_icon {
        Default,
        Error,
        Warning,
        Question,
        Asterisk,
    };

    enum class alert_response {
        OK,
        CANCEL,
        YES,
        NO,
    };

    enum class window_type {
        TOP,
        POPUP
    };

    enum class Shadow {
        NONE,
        IN,
        OUT,
        ETCHED_IN,
        ETCHED_OUT
    };

    enum class Policy {
        ALWAYS,
        AUTOMATIC,
        NEVER
    };

    namespace details {

        enum class Convert {
            Implicit, Explicit
        };

        template<class Self, class T, Convert mode>
        struct Convertible;

        template<class Self, class T>
        struct Convertible<Self, T, Convert::Implicit> {
            operator T() const
            { return reinterpret_cast<T>(static_cast<Self const *>(this)->_handle); }
        };

        template<class Self, class T>
        struct Convertible<Self, T, Convert::Explicit> {
            explicit operator T() const
            { return reinterpret_cast<T>(static_cast<Self const *>(this)->_handle); }
        };

        template<class Self, class... T>
        struct All : T ... {
            All()
            {};
        };

        template<class Self, class Interfaces>
        struct Mixin;
        template<class Self>
        struct Mixin<Self, void()> {
            using type = All<Self>;
        };
        template<class Self, class... Interfaces>
        struct Mixin<Self, void(Interfaces...)> {
            using type = All<Self, Interfaces...>;
        };
    }

    extern struct Null {} null;

    class Object :
            public details::Convertible<Object, _GtkObject *, details::Convert::Explicit>,
            public details::Convertible<Object, _GTypeInstance *, details::Convert::Explicit> {
    public:
        using self = Object *;
        using native = _GtkObject *;
        native _handle;

        Object(native h) : _handle(h)
        {}

        explicit operator bool() const
        { return _handle != nullptr; }

        explicit operator void *() const
        { return _handle; }

        template<class Lambda>
        gulong connect(char const *detailed_signal, Lambda &&c_handler, void *data);
    };
    static_assert(sizeof(Object) == sizeof(Object::native), "object slicing");

#define WRAP(name, super, T, interfaces, ctors, methods) \
    class name; \
    class I##name : public details::Convertible<name, T *, details::Convert::Implicit> { \
    public: \
        using self = name *; \
        methods \
    }; \
    class name : public super, public I##name, public details::Mixin<name, void interfaces>::type { \
    public: \
        using self = name *; \
        using native = T *; \
        explicit name(native h) : super(reinterpret_cast<super::native>(h)) {} \
        explicit name(Null n) : name((native) nullptr) {} \
        ctors \
    }; \
    inline bool operator<(name self, name other) { return self._handle < other._handle; } \
    static_assert(sizeof(name) == sizeof(super), "object slicing")

    // https://developer.gnome.org/gtk2/stable/ch01.html

    // GInterface

    WRAP(CellEditable, Object, _GtkCellEditable, (),
    ,
    );

    WRAP(Editable, Object, _GtkEditable, (),
         Editable();
    ,
         void editable(bool value);
    );

    WRAP(TreeModel, Object, _GtkTreeModel, (),
    ,
    );

    // GObject

    WRAP(Widget, Object, _GtkWidget, (),
         Widget();
    ,
         alert_response alert(
                 std::string text,
                 std::string title = "NetRadiant",
                 alert_type type = alert_type::OK,
                 alert_icon icon = alert_icon::Default
         );
         const char *file_dialog(
                 bool open,
                 const char *title,
                 const char *path = nullptr,
                 const char *pattern = nullptr,
                 bool want_load = false,
                 bool want_import = false,
                 bool want_save = false
         );
         void show();
    );

    WRAP(Container, Widget, _GtkContainer, (),
    ,
         void add(Widget widget);

         void remove(Widget widget);

         template<class Lambda>
         void foreach(Lambda &&lambda);
    );

    WRAP(Bin, Container, _GtkBin, (),
    ,
    );

    class AccelGroup;
    WRAP(Window, Bin, _GtkWindow, (),
         Window(window_type type);
    ,
         Window create_dialog_window(
                 const char *title,
                 void func(),
                 void *data,
                 int default_w = -1,
                 int default_h = -1
         );

         Window create_modal_dialog_window(
                 const char *title,
                 ModalDialog &dialog,
                 int default_w = -1,
                 int default_h = -1
         );

         Window create_floating_window(const char *title);

         std::uint64_t on_key_press(
                 bool (*f)(Widget widget, _GdkEventKey *event, void *extra),
                 void *extra = nullptr
         );

         void add_accel_group(AccelGroup group);
    );

    WRAP(Dialog, Window, _GtkDialog, (),
    ,
    );

    WRAP(Alignment, Bin, _GtkAlignment, (),
         Alignment(float xalign, float yalign, float xscale, float yscale);
    ,
    );

    WRAP(Frame, Bin, _GtkFrame, (),
         Frame(const char *label = nullptr);
    ,
    );

    WRAP(Button, Bin, _GtkButton, (),
         Button();
         Button(const char *label);
    ,
    );

    WRAP(ToggleButton, Button, _GtkToggleButton, (),
    ,
         bool active();
    );

    WRAP(CheckButton, ToggleButton, _GtkCheckButton, (),
         CheckButton(const char *label);
    ,
    );

    WRAP(RadioButton, CheckButton, _GtkRadioButton, (),
    ,
    );

    WRAP(Item, Bin, _GtkItem, (),
    ,
    );

    WRAP(MenuItem, Item, _GtkMenuItem, (),
         MenuItem();
         MenuItem(const char *label, bool mnemonic = false);
    ,
    );

    WRAP(CheckMenuItem, MenuItem, _GtkCheckMenuItem, (),
    ,
    );

    WRAP(RadioMenuItem, CheckMenuItem, _GtkRadioMenuItem, (),
    ,
    );

    WRAP(TearoffMenuItem, MenuItem, _GtkTearoffMenuItem, (),
         TearoffMenuItem();
    ,
    );

    WRAP(ComboBox, Bin, _GtkComboBox, (),
    ,
    );

    WRAP(ComboBoxText, ComboBox, _GtkComboBoxText, (),
         ComboBoxText();
    ,
    );

    WRAP(ToolItem, Bin, _GtkToolItem, (),
    ,
    );

    WRAP(ToolButton, ToolItem, _GtkToolButton, (),
    ,
    );

    WRAP(ToggleToolButton, ToolButton, _GtkToggleToolButton, (),
    ,
    );

    WRAP(RadioToolButton, ToggleToolButton, _GtkRadioToolButton, (),
    ,
    );

    WRAP(ScrolledWindow, Bin, _GtkScrolledWindow, (),
         ScrolledWindow();
    ,
    );

    WRAP(Box, Container, _GtkBox, (),
    ,
    );

    WRAP(VBox, Box, _GtkVBox, (),
         VBox(bool homogenous, int spacing);
    ,
    );

    WRAP(HBox, Box, _GtkHBox, (),
         HBox(bool homogenous, int spacing);
    ,
    );

    WRAP(Paned, Container, _GtkPaned, (),
    ,
    );

    WRAP(HPaned, Paned, _GtkHPaned, (),
         HPaned();
    ,
    );

    WRAP(VPaned, Paned, _GtkVPaned, (),
         VPaned();
    ,
    );

    WRAP(MenuShell, Container, _GtkMenuShell, (),
    ,
    );

    WRAP(MenuBar, MenuShell, _GtkMenuBar, (),
    ,
    );

    WRAP(Menu, MenuShell, _GtkMenu, (),
         Menu();
    ,
    );

    WRAP(Table, Container, _GtkTable, (),
         Table(std::size_t rows, std::size_t columns, bool homogenous);
    ,
    );

    WRAP(TextView, Container, _GtkTextView, (),
         TextView();
    ,
    );

    WRAP(Toolbar, Container, _GtkToolbar, (),
    ,
    );

    class TreeModel;
    WRAP(TreeView, Widget, _GtkTreeView, (),
         TreeView();
         TreeView(TreeModel model);
    ,
    );

    WRAP(Misc, Widget, _GtkMisc, (),
    ,
    );

    WRAP(Label, Widget, _GtkLabel, (),
         Label(const char *label);
    ,
    );

    WRAP(Image, Widget, _GtkImage, (),
         Image();
    ,
    );

    WRAP(Entry, Widget, _GtkEntry, (IEditable, ICellEditable),
         Entry();
         Entry(std::size_t max_length);
    ,
    );

    class Adjustment;
    WRAP(SpinButton, Entry, _GtkSpinButton, (),
         SpinButton(Adjustment adjustment, double climb_rate, std::size_t digits);
    ,
    );

    WRAP(Range, Widget, _GtkRange, (),
    ,
    );

    WRAP(Scale, Range, _GtkScale, (),
    ,
    );

    WRAP(HScale, Scale, _GtkHScale, (),
         HScale(Adjustment adjustment);
         HScale(double min, double max, double step);
    ,
    );

    WRAP(Adjustment, Object, _GtkAdjustment, (),
         Adjustment(double value,
                    double lower, double upper,
                    double step_increment, double page_increment,
                    double page_size);
    ,
    );

    WRAP(CellRenderer, Object, _GtkCellRenderer, (),
    ,
    );

    WRAP(CellRendererText, CellRenderer, _GtkCellRendererText, (),
         CellRendererText();
    ,
    );

    struct TreeViewColumnAttribute {
        const char *attribute;
        int column;
    };
    WRAP(TreeViewColumn, Object, _GtkTreeViewColumn, (),
         TreeViewColumn(const char *title, CellRenderer renderer, std::initializer_list<TreeViewColumnAttribute> attributes);
    ,
    );

    WRAP(AccelGroup, Object, _GtkAccelGroup, (),
         AccelGroup();
    ,
    );

    WRAP(EntryCompletion, Object, _GtkEntryCompletion, (),
    ,
    );

    WRAP(ListStore, Object, _GtkListStore, (ITreeModel),
    ,
         void clear();
    );

    WRAP(TreeSelection, Object, _GtkTreeSelection, (),
    ,
    );

    // GBoxed

    WRAP(TreePath, Object, _GtkTreePath, (),
         TreePath();
         TreePath(const char *path);
    ,
    );

#undef WRAP

    // callbacks

    namespace {
        using GtkCallback = void (*)(_GtkWidget *, void *);
        extern "C" {
        void gtk_container_foreach(_GtkContainer *, GtkCallback, void *);
        }
    }

#define this (*static_cast<self>(this))

    template<class Lambda>
    gulong Object::connect(char const *detailed_signal, Lambda &&c_handler, void *data)
    {
        return g_signal_connect(G_OBJECT(this), detailed_signal, c_handler, data);
    }

    template<class Lambda>
    void IContainer::foreach(Lambda &&lambda)
    {
        GtkCallback cb = [](_GtkWidget *widget, void *data) -> void {
            using Function = typename std::decay<Lambda>::type;
            Function *f = static_cast<Function *>(data);
            (*f)(Widget(widget));
        };
        gtk_container_foreach(this, cb, &lambda);
    }

#undef this

}

#endif
