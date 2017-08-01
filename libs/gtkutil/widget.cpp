#include "widget.h"
#include <gtk/gtk.h>

void widget_set_visible(ui::Widget widget, bool shown)
{
    if (shown) {
        widget.show();
    } else {
        gtk_widget_hide(widget);
    }
}

bool widget_is_visible(ui::Widget widget)
{
    return gtk_widget_get_visible(widget) != FALSE;
}

void widget_queue_draw(ui::Widget &widget)
{
    gtk_widget_queue_draw(widget);
}

void widget_make_default(ui::Widget widget)
{
    gtk_widget_set_can_default(widget, true);
    gtk_widget_grab_default(widget);
}

gboolean ToggleShown::notify_visible(ui::Widget widget, gpointer dummy, ToggleShown *self)
{
    self->update();
    return FALSE;
}

gboolean ToggleShown::destroy(ui::Widget widget, ToggleShown *self)
{
    self->m_shownDeferred = gtk_widget_get_visible(self->m_widget) != FALSE;
    self->m_widget = ui::Widget(ui::null);
    return FALSE;
}

void ToggleShown::update()
{
    m_item.update();
}

bool ToggleShown::active() const
{
    if (!m_widget) {
        return m_shownDeferred;
    } else {
        return gtk_widget_get_visible(m_widget) != FALSE;
    }
}

void ToggleShown::exportActive(const BoolImportCallback &importCallback)
{
    importCallback(active());
}

void ToggleShown::set(bool shown)
{
    if (!m_widget) {
        m_shownDeferred = shown;
    } else {
        widget_set_visible(m_widget, shown);
    }
}

void ToggleShown::toggle()
{
    widget_toggle_visible(m_widget);
}

void ToggleShown::connect(ui::Widget widget)
{
    m_widget = widget;
    widget_set_visible(m_widget, m_shownDeferred);
    m_widget.connect("notify::visible", G_CALLBACK(notify_visible), this);
    m_widget.connect("destroy", G_CALLBACK(destroy), this);
    update();
}

gboolean WidgetFocusPrinter::focus_in(ui::Widget widget, GdkEventFocus *event, WidgetFocusPrinter *self)
{
    globalOutputStream() << self->m_name << " takes focus\n";
    return FALSE;
}

gboolean WidgetFocusPrinter::focus_out(ui::Widget widget, GdkEventFocus *event, WidgetFocusPrinter *self)
{
    globalOutputStream() << self->m_name << " loses focus\n";
    return FALSE;
}

void WidgetFocusPrinter::connect(ui::Widget widget)
{
    widget.connect("focus_in_event", G_CALLBACK(focus_in), this);
    widget.connect("focus_out_event", G_CALLBACK(focus_out), this);
}
