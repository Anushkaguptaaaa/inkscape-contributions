// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors: Tavmjong Bah
// Mike Kowalski
//

#include "ink-spin-button.h"

#include <cassert>
#include <iomanip>

#include "ui/containerize.h"
#include "ui/util.h"
#include "util/expression-evaluator.h"

namespace Inkscape::UI::Widget {

// CSS styles for InkSpinButton
// language=CSS
auto ink_spinbutton_css = R"=====(
@define-color border-color @unfocused_borders;
@define-color bgnd-color alpha(@theme_base_color, 1.0);
@define-color focus-color alpha(@theme_selected_bg_color, 0.5);
/* :root { --border-color: lightgray; } - this is not working yet, so using nonstandard @define-color */
#InkSpinButton { border: 0 solid @border-color; border-radius: 2px; background-color: @bgnd-color; }
#InkSpinButton.frame { border: 1px solid @border-color; }
#InkSpinButton:hover button { opacity: 1; }
#InkSpinButton:focus-within { outline: 2px solid @focus-color; outline-offset: -2px; }
#InkSpinButton label#InkSpinButton-Label { opacity: 0.5; margin-left: 3px; margin-right: 3px; }
#InkSpinButton button { border: 0 solid alpha(@border-color, 0.30); border-radius: 2px; padding: 1px; min-width: 6px; min-height: 8px; -gtk-icon-size: 10px; background-image: none; }
#InkSpinButton button.left  { border-top-right-radius: 0; border-bottom-right-radius: 0; border-right-width: 1px; }
#InkSpinButton button.right { border-top-left-radius: 0; border-bottom-left-radius: 0; border-left-width: 1px; }
#InkSpinButton entry#InkSpinButton-Entry { border: none; border-radius: 3px; padding: 0; min-height: 13px; background-color: @bgnd-color; outline-width: 0; }
)=====";
constexpr int timeout_click = 500;
constexpr int timeout_repeat = 50;

static Glib::RefPtr<Gdk::Cursor> g_resizing_cursor;
static Glib::RefPtr<Gdk::Cursor> g_text_cursor;

void InkSpinButton::construct() {
    set_name("InkSpinButton");

    set_overflow(Gtk::Overflow::HIDDEN);

    _minus.set_name("InkSpinButton-Minus");
    _minus.add_css_class("left");
    _value.set_name("InkSpinButton-Value");
    _plus.set_name("InkSpinButton-Plus");
    _plus.add_css_class("right");
    _entry.set_name("InkSpinButton-Entry");
    _entry.set_alignment(0.5f);
    _entry.set_max_width_chars(3); // let it shrink, we can always stretch it
    _label.set_name("InkSpinButton-Label");

    _value.set_expand();
    _entry.set_expand();

    _minus.set_margin(0);
    _minus.set_size_request(8, -1);
    _value.set_margin(0);
    _value.set_single_line_mode();
    _value.set_overflow(Gtk::Overflow::HIDDEN);
    _plus.set_margin(0);
    _plus.set_size_request(8, -1);
    _minus.set_can_focus(false);
    _plus.set_can_focus(false);
    _label.set_can_focus(false);
    _label.set_xalign(0.0f);
    _label.set_visible(false);

    _minus.set_icon_name("go-previous-symbolic");
    _plus.set_icon_name("go-next-symbolic");

    containerize(*this);
    _label.insert_at_end(*this);
    _minus.insert_at_end(*this);
    _value.insert_at_end(*this);
    _entry.insert_at_end(*this);
    _plus.insert_at_end(*this);

    set_focus_child(_entry);

    static Glib::RefPtr<Gtk::CssProvider> provider;
    if (!provider) {
        provider = Gtk::CssProvider::create();
        provider->load_from_data(ink_spinbutton_css);
        auto const display = Gdk::Display::get_default();
        Gtk::StyleContext::add_provider_for_display(display, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 10);
    }

    // ------------- CONTROLLERS -------------

    // This is mouse movement. Shows/hides +/- buttons.
    // Shows/hides +/- buttons.
    _motion = Gtk::EventControllerMotion::create();
    _motion->signal_enter().connect(sigc::mem_fun(*this, &InkSpinButton::on_motion_enter));
    _motion->signal_leave().connect(sigc::mem_fun(*this, &InkSpinButton::on_motion_leave));
    add_controller(_motion);

    // This is mouse movement. Sets cursor.
    _motion_value = Gtk::EventControllerMotion::create();
    _motion_value->signal_enter().connect(sigc::mem_fun(*this, &InkSpinButton::on_motion_enter_value));
    _motion_value->signal_leave().connect(sigc::mem_fun(*this, &InkSpinButton::on_motion_leave_value));
    _value.add_controller(_motion_value);

    // This is mouse drag movement. Changes value.
    _drag_value = Gtk::GestureDrag::create();
    _drag_value->signal_begin().connect(sigc::mem_fun(*this, &InkSpinButton::on_drag_begin_value));
    _drag_value->signal_update().connect(sigc::mem_fun(*this, &InkSpinButton::on_drag_update_value));
    _drag_value->signal_end().connect(sigc::mem_fun(*this, &InkSpinButton::on_drag_end_value));
    _drag_value->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    _value.add_controller(_drag_value);

    // Changes value.
    _scroll = Gtk::EventControllerScroll::create();
    _scroll->signal_scroll_begin().connect(sigc::mem_fun(*this, &InkSpinButton::on_scroll_begin));
    _scroll->signal_scroll().connect(sigc::mem_fun(*this, &InkSpinButton::on_scroll), false);
    _scroll->signal_scroll_end().connect(sigc::mem_fun(*this, &InkSpinButton::on_scroll_end));
    _scroll->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES); // Mouse wheel is on y.
    add_controller(_scroll);

    _click_minus = Gtk::GestureClick::create();
    _click_minus->signal_pressed().connect(sigc::mem_fun(*this, &InkSpinButton::on_pressed_minus));
    _click_minus->signal_released().connect([this](int, double, double){ stop_spinning(); });
    _click_minus->signal_unpaired_release().connect([this](auto, auto, auto, auto){ stop_spinning(); });
    _click_minus->set_propagation_phase(Gtk::PropagationPhase::CAPTURE); // Steal from default handler.
    _minus.add_controller(_click_minus);

    _click_plus = Gtk::GestureClick::create();
    _click_plus->signal_pressed().connect(sigc::mem_fun(*this, &InkSpinButton::on_pressed_plus));
    _click_plus->signal_released().connect([this](int, double, double){ stop_spinning(); });
    _click_plus->signal_unpaired_release().connect([this](auto, auto, auto, auto){ stop_spinning(); });
    _click_plus->set_propagation_phase(Gtk::PropagationPhase::CAPTURE); // Steal from default handler.
    _plus.add_controller(_click_plus);

    _focus = Gtk::EventControllerFocus::create();
    _focus->signal_enter().connect([this]() {
        // show editable button if '*this' is focused, but not its entry
        if (_focus->is_focus()) {
            set_focusable(false);
            enter_edit();
        }
    });
    _focus->signal_leave().connect([this]() {
        if (_entry.is_visible()) {
            commit_entry();
        }
        exit_edit();
        set_focusable(true);
    });
    add_controller(_focus);
    _entry.set_focus_on_click(false);
    _entry.set_focusable(false);
    _entry.set_can_focus();
    set_can_focus();
    set_focusable();
    set_focus_on_click();

    _key_entry = Gtk::EventControllerKey::create();
    _key_entry->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    _key_entry->signal_key_pressed().connect(sigc::mem_fun(*this, &InkSpinButton::on_key_pressed), false); // Before default handler.
    _entry.add_controller(_key_entry);

    //                  GTK4
    // -------------   SIGNALS   -------------

    _entry.signal_activate().connect([this] { on_activate(); });

    // Value (button) NOT USED, Click handled by zero length drag.
    // m_value->signal_clicked().connect(sigc::mem_fun(*this, &SpinButton::on_value_clicked));

    _minus.set_visible();
    auto m = _minus.measure(Gtk::Orientation::HORIZONTAL);
    _button_width = m.sizes.natural;
    m = _entry.measure(Gtk::Orientation::VERTICAL);
    _entry_height = m.sizes.natural;
    _baseline = m.baselines.natural;

    set_value(_num_value);
    set_step(_step_value);
    set_has_frame(_has_frame);
    set_has_arrows(_show_arrows);
    set_scaling_factor(_scaling_factor);
    show_arrows(false);
    _entry.hide();

    property_label().signal_changed().connect([this] {
        set_label(_label_text.get_value().raw());
    });
    set_label(_label_text.get_value());

    property_digits().signal_changed().connect([this]{ queue_resize(); update(); });
    property_has_frame().signal_changed().connect([this]{ set_has_frame(_has_frame); });
    property_show_arrows().signal_changed().connect([this]{ set_has_arrows(_show_arrows); });
    property_scaling_factor().signal_changed().connect([this]{ set_scaling_factor(_scaling_factor); });
    property_step_value().signal_changed().connect([this]{ set_step(_step_value); });
    property_min_value().signal_changed().connect([this]{ _adjustment->set_lower(_min_value); });
    property_max_value().signal_changed().connect([this]{ _adjustment->set_upper(_max_value); });
    property_value().signal_changed().connect([this]{ set_value(_num_value); });
    property_prefix().signal_changed().connect([this]{ update(); });
    property_suffix().signal_changed().connect([this]{ update(); });

    _connection = _adjustment->signal_value_changed().connect([this](){ update(); });
    update();
}

#define INIT_PROPERTIES \
    _adjust(*this, "adjustment", Gtk::Adjustment::create(0, 0, 100, 1)), \
    _digits(*this, "digits", 3), \
    _num_value(*this, "value", 0.0), \
    _min_value(*this, "min-value", 0.0), \
    _max_value(*this, "max-value", 100.0), \
    _step_value(*this, "step-value", 1.0), \
    _scaling_factor(*this, "scaling-factor", 1.0), \
    _has_frame(*this, "has-frame", true), \
    _show_arrows(*this, "show-arrows", true), \
    _enter_exit(*this, "enter-exit-editing", false), \
    _label_text(*this, "label", {}), \
    _prefix(*this, "prefix", {}), \
    _suffix(*this, "suffix", {})


InkSpinButton::InkSpinButton():
    Glib::ObjectBase("InkSpinButton"),
    INIT_PROPERTIES {

    construct();
}

InkSpinButton::InkSpinButton(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    Glib::ObjectBase("InkSpinButton"),
    Gtk::Widget(cobject),
    INIT_PROPERTIES {

    construct();
}

InkSpinButton::InkSpinButton(BaseObjectType* cobject):
    Glib::ObjectBase("InkSpinButton"),
    Gtk::Widget(cobject),
    INIT_PROPERTIES {

     construct();
}

#undef INIT_PROPERTIES

InkSpinButton::~InkSpinButton() = default;

Gtk::SizeRequestMode InkSpinButton::get_request_mode_vfunc() const {
    return Gtk::Widget::get_request_mode_vfunc();
}

void InkSpinButton::measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural, int& minimum_baseline, int& natural_baseline) const {

    std::string text;
    if (_min_size_pattern.empty()) {
        auto delta = _digits.get_value() > 0 ? pow(10.0, -_digits.get_value()) : 0;
        auto low = format(_adjustment->get_lower() + delta, true, false, true, true);
        auto high = format(_adjustment->get_upper() - delta, true, false, true, true);
        text = low.size() > high.size() ? low : high;
    }
    else {
        text = _min_size_pattern;
    }

    // http://developer.gnome.org/pangomm/unstable/classPango_1_1Layout.html
    auto layout = const_cast<InkSpinButton*>(this)->create_pango_layout("\u2009" + text + "\u2009");

    int text_width = 0;
    int text_height = 0;
    // get the text dimensions
    layout->get_pixel_size(text_width, text_height);

    if (orientation == Gtk::Orientation::HORIZONTAL) {
        minimum_baseline = natural_baseline = -1;
        // always measure, so gtk doesn't complain
        auto m = _minus.measure(orientation);
        auto p = _plus.measure(orientation);
        auto _ = _entry.measure(orientation);
        _ = _value.measure(orientation);
        _ = _label.measure(orientation);

        auto btn = _enable_arrows ? _button_width : 0;
        // always reserve space for inc/dec buttons and label, whichever is greater
        minimum = natural = std::max(_label_width + text_width, btn + text_width + btn);
    }
    else {
        minimum_baseline = natural_baseline = _baseline;
        auto height = std::max(text_height, _entry_height);
        minimum = height;
        natural = std::max(static_cast<int>(1.5 * text_height), _entry_height);
    }
}

void InkSpinButton::size_allocate_vfunc(int width, int height, int baseline) {
    Gtk::Allocation allocation;
    allocation.set_height(height);
    allocation.set_width(_button_width);
    allocation.set_x(0);
    allocation.set_y(0);

    int left = 0;
    int right = width;

    // either label or buttons may be visible, but not both
    if (_label.get_visible()) {
        Gtk::Allocation alloc;
        alloc.set_height(height);
        alloc.set_width(_label_width);
        alloc.set_x(0);
        alloc.set_y(0);
        _label.size_allocate(alloc, baseline);
        left += _label_width;
        right -= _label_width;
    }
    if (_minus.get_visible()) {
        _minus.size_allocate(allocation, baseline);
        left += allocation.get_width();
    }
    if (_plus.get_visible()) {
        allocation.set_x(width - allocation.get_width());
        _plus.size_allocate(allocation, baseline);
        right -= allocation.get_width();
    }

    allocation.set_x(left);
    allocation.set_width(std::max(0, right - left));
    if (_value.get_visible()) {
        _value.size_allocate(allocation, baseline);
    }
    if (_entry.get_visible()) {
        _entry.size_allocate(allocation, baseline);
    }
}


Glib::RefPtr<Gtk::Adjustment>& InkSpinButton::get_adjustment() {
    return _adjustment;
}

void InkSpinButton::set_adjustment(const Glib::RefPtr<Gtk::Adjustment>& adjustment) {
    if (!adjustment) return;

    _connection.disconnect();
    _adjustment = adjustment;
    _connection = _adjustment->signal_value_changed().connect([this](){ update(); });
    update();
}

void InkSpinButton::set_digits(int digits) {
    _digits = digits;
    update();
}

int InkSpinButton::get_digits() const {
    return _digits.get_value();
}

void InkSpinButton::set_range(double min, double max) {
    _adjustment->set_lower(min);
    _adjustment->set_upper(max);
}

void InkSpinButton::set_step(double step_increment) {
    _adjustment->set_step_increment(step_increment);
}

void InkSpinButton::set_prefix(const std::string& prefix, bool add_space) {
    if (add_space && !prefix.empty()) {
        _prefix.set_value(prefix + " ");
    }
    else {
        _prefix.set_value(prefix);
    }
    update();
}

void InkSpinButton::set_suffix(const std::string& suffix, bool add_half_space) {
    if (add_half_space && !suffix.empty()) {
        // thin space
        _suffix.set_value("\u2009" + suffix);
    }
    else {
        _suffix.set_value(suffix);
    }
    update();
}

void InkSpinButton::set_has_frame(bool frame) {
    if (frame) {
        add_css_class("frame");
    }
    else {
        remove_css_class("frame");
    }
}

void InkSpinButton::set_trim_zeros(bool trim) {
    if (_trim_zeros != trim) {
        _trim_zeros = trim;
        update();
    }
}

void InkSpinButton::set_scaling_factor(double factor) {
    assert(factor > 0 && factor < 1e9);
    _fmt_scaling_factor = factor;
    queue_resize();
    update();
}

static void trim_zeros(std::string& ret) {
    while (ret.find('.') != std::string::npos &&
        (ret.substr(ret.length() - 1, 1) == "0" || ret.substr(ret.length() - 1, 1) == ".")) {
        ret.pop_back();
    }
}

std::string InkSpinButton::format(double value, bool with_prefix_suffix, bool with_markup, bool trim_zeros, bool limit_size) const {
    std::stringstream ss;
    ss.imbue(std::locale("C"));
    std::string number;
    if (value > 1e12 || value < -1e12) {
        // use scientific notation to limit size of the output number
        ss << std::scientific << std::setprecision(std::numeric_limits<double>::digits10) << value;
        number = ss.str();
    }
    else {
        ss << std::fixed << std::setprecision(_digits.get_value()) << value;
        number = ss.str();
        if (trim_zeros) {
            UI::Widget::trim_zeros(number);
        }
        if (limit_size) {
            auto limit = std::numeric_limits<double>::digits10;
            if (value < 0) limit += 1;

            if (number.size() > limit) {
                number = number.substr(0, limit);
            }
        }
    }

    auto suffix = _suffix.get_value();
    auto prefix = _prefix.get_value();
    if (with_prefix_suffix && (!suffix.empty() || !prefix.empty())) {
        if (with_markup) {
            std::stringstream markup;
            if (!prefix.empty()) {
                markup << "<span alpha='50%'>" << Glib::Markup::escape_text(prefix) << "</span>";
            }
            markup << "<span>" << number << "</span>";
            if (!suffix.empty()) {
                markup << "<span alpha='50%'>" << Glib::Markup::escape_text(suffix) << "</span>";
            }
            return markup.str();
        }
        else {
            return prefix + number + suffix;
        }
    }

    return number;
}

void InkSpinButton::update(bool fire_change_notification) {
    if (!_adjustment) return;

    auto value = _adjustment->get_value();
    auto text = format(value, false, false, _trim_zeros, false);
    _entry.set_text(text);
    if (_suffix.get_value().empty() && _prefix.get_value().empty()) {
        _value.set_text(text);
    }
    else {
        _value.set_markup(format(value, true, true, _trim_zeros, false));
    }

    _minus.set_sensitive(_adjustment->get_value() > _adjustment->get_lower());
    _plus.set_sensitive(_adjustment->get_value() < _adjustment->get_upper());

    if (fire_change_notification) {
        _signal_value_changed.emit(value / _fmt_scaling_factor);
    }
}

void InkSpinButton::set_new_value(double new_value) {
    _adjustment->set_value(new_value);
    //TODO: reflect new value in _num_value property while avoiding cycle updates
}

// ---------------- CONTROLLERS -----------------

// ------------------  MOTION  ------------------

void InkSpinButton::on_motion_enter(double x, double y) {
    if (_focus->contains_focus()) return;

    show_label(false);
    show_arrows();
}

void InkSpinButton::on_motion_leave() {
    if (_focus->contains_focus()) return;

    show_arrows(false);
    show_label();

    if (_entry.is_visible()) {
        // We left spinbutton, save value and update.
        commit_entry();
        exit_edit();
    }
}

// ---------------  MOTION VALUE  ---------------

void InkSpinButton::on_motion_enter_value(double x, double y) {
    _old_cursor = get_cursor();
    if (!g_resizing_cursor) {
        g_resizing_cursor = Gdk::Cursor::create(Glib::ustring("ew-resize"));
        g_text_cursor = Gdk::Cursor::create(Glib::ustring("text"));
    }
    // if draging/scrolling adjustment is enabled, show appropriate cursor
    if (_drag_full_travel > 0) {
        _current_cursor = g_resizing_cursor;
        set_cursor(_current_cursor);
    }
    else {
        _current_cursor = g_text_cursor;
        set_cursor(_current_cursor);
    }
}

void InkSpinButton::on_motion_leave_value() {
    _current_cursor = _old_cursor;
    set_cursor(_current_cursor);
}

// ---------------   DRAG VALUE  ----------------

static double get_accel_factor(Gdk::ModifierType state) {
    double scale = 1.0;
    // Ctrl modifier slows down, Shift speeds up
    if ((state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK) {
        scale = 0.1;
    } else if ((state & Gdk::ModifierType::SHIFT_MASK) == Gdk::ModifierType::SHIFT_MASK) {
        scale = 10.0;
    }
    return scale;
}

void InkSpinButton::on_drag_begin_value(Gdk::EventSequence* sequence) {
    _initial_value = _adjustment->get_value();
    _drag_value->get_point(sequence, _drag_start.x,_drag_start.y);
}

void InkSpinButton::on_drag_update_value(Gdk::EventSequence* sequence) {
    if (_drag_full_travel <= 0) return;

    double dx = 0.0;
    double dy = 0.0;
    _drag_value->get_offset(dx, dy);

    // If we don't move, then it probably was a button click.
    auto delta = 1; // tweak this value to reject real clicks, or else we'll change value inadvertently
    if (std::abs(dx) > delta || std::abs(dy) > delta) {
        auto max_dist = _drag_full_travel; // distance to travel to adjust full range
        auto range = _adjustment->get_upper() - _adjustment->get_lower();
        auto state = _drag_value->get_current_event_state();
        auto distance = std::hypot(dx, dy);
        auto angle = std::atan2(dx, dy);
        auto grow = angle > M_PI_4 || angle < -M_PI+M_PI_4;
        if (!grow) distance = -distance;

        auto value = _initial_value + get_accel_factor(state) * distance / max_dist * range + _adjustment->get_lower();
        set_new_value(value);
        _dragged = true;
    }
}

void InkSpinButton::on_drag_end_value(Gdk::EventSequence* sequence) {
    double dx = 0.0;
    double dy = 0.0;
    _drag_value->get_offset(dx, dy);

    if (dx == 0 && !_dragged) {
        // Must have been a click!
        enter_edit();
    }
    _dragged = false;
}

void InkSpinButton::show_arrows(bool on) {
    _minus.set_visible(on && _enable_arrows);
    _plus.set_visible(on && _enable_arrows);
}

void InkSpinButton::show_label(bool on) {
    _label.set_visible(on && _label_width > 0);
}

static char const *get_text(Gtk::Editable const &editable) {
    return gtk_editable_get_text(const_cast<GtkEditable *>(editable.gobj())); // C API is const-incorrect
}

bool InkSpinButton::commit_entry() {
    try {
        double value = 0.0;
        auto text = get_text(_entry);
        if (_dont_evaluate) {
            value = std::stod(text);
        }
        else if (_evaluator) {
            value = _evaluator(text);
        }
        else {
            value = Util::ExpressionEvaluator{text}.evaluate().value;
        }
        _adjustment->set_value(value);
        return true;
    }
    catch (const std::exception& e) {
        g_message("Expression error: %s", e.what());
    }
    return false;
}

void InkSpinButton::exit_edit() {
    show_arrows(false);
    _entry.hide();
    show_label();
    _value.show();
}

void InkSpinButton::cancel_editing() {
    update(false); // take current recorder value and update text/display
    exit_edit();
}

inline void InkSpinButton::enter_edit() {
    show_arrows(false);
    show_label(false);
    stop_spinning();
    _value.hide();
    _entry.select_region(0, _entry.get_text_length());
    _entry.show();
    // postpone it, it won't work immediately:
    Glib::signal_idle().connect_once([this](){_entry.grab_focus();}, Glib::PRIORITY_HIGH_IDLE);
}

bool InkSpinButton::defocus() {
    if (_focus->contains_focus()) {
        // move focus away
        if (_defocus_widget) {
            if (_defocus_widget->grab_focus()) return true;
        }
        if (_entry.child_focus(Gtk::DirectionType::TAB_FORWARD)) {
            return true;
        }
        if (auto root = get_root()) {
            root->unset_focus();
            return true;
        }
    }
    return false;
}

// ------------------  SCROLL  ------------------

void InkSpinButton::on_scroll_begin() {
    if (_drag_full_travel <= 0) return;

    _scroll_counter = 0;
    set_cursor("none");
}

bool InkSpinButton::on_scroll(double dx, double dy) {
    if (_drag_full_travel <= 0) return false;

    // grow direction: up or right
    auto delta = std::abs(dx) > std::abs(dy) ? -dx : dy;
    _scroll_counter += delta;
    // this is a threshold to control rate at which scrolling increments/decrements current value;
    // the larger the threshold, the slower the rate; it may need to be tweaked on different platforms
#ifdef _WIN32
    // default for mouse wheel on windows
    constexpr double threshold = 1.0;
#elif defined __APPLE__
    // scrolling is very sensitive on macOS
    constexpr double threshold = 5.0;
#else
    //todo: default for Linux
    constexpr double threshold = 1.0;
#endif
    if (std::abs(_scroll_counter) >= threshold) {
        auto inc = std::round(_scroll_counter / threshold);
        _scroll_counter = 0;
        auto state = _scroll->get_current_event_state();
        change_value(inc, state);
    }
    return true;
}

void InkSpinButton::on_scroll_end() {
    if (_drag_full_travel <= 0) return;

    _scroll_counter = 0;
    set_cursor(_current_cursor);
}

void InkSpinButton::set_value(double new_value) {
    set_new_value(new_value * _fmt_scaling_factor);
}

double InkSpinButton::get_value() const {
    return _adjustment->get_value() / _fmt_scaling_factor;
}

void InkSpinButton::change_value(double inc, Gdk::ModifierType state) {
    double scale = get_accel_factor(state);
    set_new_value(_adjustment->get_value() + _adjustment->get_step_increment() * scale * inc);
}

// ------------------   KEY    ------------------

bool InkSpinButton::on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state) {
   switch (keyval) {
   case GDK_KEY_Escape: // Cancel
       // Esc pressed - cancel editing
       cancel_editing();
       defocus();
       return false; // allow Esc to be handled by dialog too

   // signal "activate" uses this key, so we won't see it:
   // case GDK_KEY_Return:
       // break;

   case GDK_KEY_Up:
       change_value(1, state);
       return true;

   case GDK_KEY_Down:
       change_value(-1, state);
       return true;

   default:
       break;
   }

   return false;
}

// ------------------  CLICK   ------------------

void InkSpinButton::on_pressed_plus(int n_press, double x, double y) {
    auto state = _click_plus->get_current_event_state();
    double inc = (state & Gdk::ModifierType::BUTTON3_MASK) == Gdk::ModifierType::BUTTON3_MASK ? 5 : 1;
    change_value(inc, state);
    start_spinning(inc, state, _click_plus);
}

void InkSpinButton::on_pressed_minus(int n_press, double x, double y) {
    auto state = _click_minus->get_current_event_state();
    double inc = (state & Gdk::ModifierType::BUTTON3_MASK) == Gdk::ModifierType::BUTTON3_MASK ? 5 : 1;
    change_value(-inc, state);
    start_spinning(-inc, state, _click_minus);
}

void InkSpinButton::on_activate() {
    bool ok = commit_entry();
    if (ok && _enter_exit_edit) {
        set_focusable(true);
        defocus();
        exit_edit();
    }
}

void InkSpinButton::on_changed() {
    // NOT USED
}

void InkSpinButton::on_editing_done() {
    // NOT USED
}

void InkSpinButton::start_spinning(double steps, Gdk::ModifierType state, Glib::RefPtr<Gtk::GestureClick>& gesture) {
    _spinning = Glib::signal_timeout().connect([=,this]() {
        change_value(steps, state);
        // speed up
        _spinning = Glib::signal_timeout().connect([=,this]() {
            change_value(steps, state);
            //TODO: find a way to read mouse button state
            auto active = gesture->is_active();
            auto btn = gesture->get_current_button();
            if (!active || !btn) return false;
            return true;
        }, timeout_repeat);
        return false;
    }, timeout_click);
}

void InkSpinButton::stop_spinning() {
    if (_spinning) _spinning.disconnect();
}

void InkSpinButton::set_drag_sensitivity(double distance) {
    _drag_full_travel = distance;
}

void InkSpinButton::set_label(const std::string& label) {
    _label.set_text(label);
    if (label.empty()) {
        _label.set_visible(false);
        _label_width = 0;
    }
    else {
        _label.set_visible(true);
        auto l = _label.measure(Gtk::Orientation::HORIZONTAL);
        _label_width = l.sizes.minimum;
    }
}

sigc::signal<void(double)> InkSpinButton::signal_value_changed() const {
    return _signal_value_changed;
}

void InkSpinButton::set_min_size(const std::string& pattern) {
    _min_size_pattern = pattern;
    queue_resize();
}

void InkSpinButton::set_evaluator_function(std::function<double(const Glib::ustring&)> cb) {
    _evaluator = cb;
}

void InkSpinButton::set_has_arrows(bool enable) {
    if (_enable_arrows == enable) return;

    _enable_arrows = enable;
    queue_resize();
    show_arrows(enable);
}

void InkSpinButton::set_enter_exit_edit(bool enable) {
    _enter_exit_edit = enable;
}

GType InkSpinButton::gtype = 0;

} // namespace Inkscape::UI::Widget
