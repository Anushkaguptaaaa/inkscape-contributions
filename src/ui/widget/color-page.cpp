// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Build a set of color page for a given color space
 *//*
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/color-page.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

#include <utility>

#include "color-wheel-factory.h"
#include "ink-color-wheel.h"
#include "ink-spin-button.h"
#include "colors/color.h"
#include "colors/color-set.h"
#include "colors/spaces/base.h"
#include "colors/spaces/components.h"
#include "ui/builder-utils.h"
#include "util/signal-blocker.h"

namespace Inkscape::UI::Widget {

static unsigned int MAX_COMPONENTS = 6;

ColorPage::ColorPage(std::shared_ptr<Space::AnySpace> space, std::shared_ptr<ColorSet> colors)
    : Gtk::Box()
    , _builder(create_builder("color-page.glade"))
    , _space(std::move(space))
    , _selected_colors(std::move(colors))
    , _specific_colors(std::make_shared<Colors::ColorSet>(_space, true))
    , _expander(get_widget<Gtk::Expander>(_builder, "wheel-expander"))
{
    set_name("ColorPage");
    append(get_widget<Gtk::Grid>(_builder, "color-page"));

    // Keep the selected colorset in-sync with the space specific colorset.
    _specific_changed_connection = _specific_colors->signal_changed.connect([this]() {
        auto scoped = SignalBlocker{_selected_changed_connection};
        for (auto &[id, color] : *_specific_colors) {
            _selected_colors->set(id, color);
        }
        if (_color_wheel && _color_wheel->get_widget().is_drawable()) {
            _color_wheel->set_color(_specific_colors->getAverage());
        }
    });

    // Keep the child in-sync with the selected colorset.
    _selected_changed_connection = _selected_colors->signal_changed.connect([this]() {
        auto scoped = SignalBlocker{_specific_changed_connection};
        for (auto &[id, color] : *_selected_colors) {
            _specific_colors->set(id, color);
        }
    });

    // Control signals when widget isn't mapped (not visible to the user)
    signal_map().connect([this]() {
        _specific_colors->setAll(*_selected_colors);
        _specific_changed_connection.unblock();
        _selected_changed_connection.unblock();
    });

    signal_unmap().connect([this]() {
        _specific_colors->clear();
        _specific_changed_connection.block();
        _selected_changed_connection.block();
    });

    // init ink spin button once before builder-derived instance is created to register its custom type first
    //todo: not working after all...
    // InkSpinButton dummy;

    for (auto &component : _specific_colors->getComponents()) {
        std::string index = std::to_string(component.index + 1);
        _channels.emplace_back(std::make_unique<ColorPageChannel>(
            _specific_colors,
            get_widget<Gtk::Label>(_builder, ("label" + index).c_str()),
            get_derived_widget<ColorSlider>(_builder, ("slider" + index).c_str(), _specific_colors, component),
            get_derived_widget<InkSpinButton>(_builder, ("spin" + index).c_str())
        ));
    }

    // Hide unncessary channel widgets
    for (auto j = _specific_colors->getComponents().size(); j < MAX_COMPONENTS; j++) {
        std::string index = std::to_string(j+1);
        hide_widget(_builder, "label" + index);
        hide_widget(_builder, "slider" + index);
        hide_widget(_builder, "spin" + index);
        hide_widget(_builder, "separator" + index);
    }

    // Color wheel
    auto wheel_type = _specific_colors->getComponents().get_wheel_type();
    // there are only a few types of color wheel supported:
    if (can_create_color_wheel(wheel_type)) {
        _expander.property_expanded().signal_changed().connect([this, wheel_type]() {
            auto on = _expander.property_expanded().get_value();
            if (on && !_color_wheel) {
                // create color wheel now
                _color_wheel = create_managed_color_wheel(wheel_type);
                _expander.set_child(_color_wheel->get_widget());
                _color_wheel->set_color(_specific_colors->getAverage());
                _color_wheel_changed = _color_wheel->connect_color_changed([this](const Color& color) {
                    auto scoped = SignalBlocker{_color_wheel_changed};
                    _specific_colors->setAll(color);
                });
            }
            if (_color_wheel) {
                if (on) {
                    // update, wheel may be stale if it was hidden
                    _color_wheel->set_color(_specific_colors->getAverage());
                }
            }
        });
    }
    else {
        _expander.set_visible(false);
    }
}

ColorPageChannel::ColorPageChannel(
    std::shared_ptr<Colors::ColorSet> color,
    Gtk::Label &label,
    ColorSlider &slider,
    InkSpinButton &spin)
    : _label(label)
    , _slider(slider)
    , _spin(spin)
    , _adj(spin.get_adjustment()), _color(std::move(color))
{
    auto &component = _slider._component;
    _label.set_markup_with_mnemonic(component.name);
    _label.set_tooltip_text(component.tip);

    _slider.set_hexpand(true);

    _adj->set_lower(0.0);
    _adj->set_upper(component.scale);
    _adj->set_page_increment(0.0);
    _adj->set_page_size(0.0);

    _spin.set_has_frame(false);
    _spin.set_digits(0);
    _spin.set_adjustment(_adj);

    _color_changed = _color->signal_changed.connect([this]() {
        if (_color->isValid(_slider._component)) {
            _adj->set_value(_slider.getScaled());
        }
    });
    _adj_changed = _adj->signal_value_changed().connect([this]() {
        auto scoped = SignalBlocker{_slider_changed};
        _slider.setScaled(_adj->get_value());
    });
    _slider_changed = _slider.signal_value_changed.connect([this]() {
        auto scoped = SignalBlocker{_adj_changed};
        _adj->set_value(_slider.getScaled());
    });
}

} // namespace Inkscape::UI::Widget

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8: textwidth=99:
