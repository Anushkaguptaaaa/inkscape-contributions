/*
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/array.h"
#include "helper-fns.h"
#include <2geom/coord.h>
#include <2geom/point.h>

namespace Inkscape {

namespace LivePathEffect {

//TODO: move maybe to svg-lenght.cpp
unsigned int
sp_svg_satellite_read_d(gchar const *str, Satellite *sat){
    if (!str) {
        return 0;
    }
    gchar ** strarray = g_strsplit(str, ",", 8);
    if(strlen(str) > 0 && strarray[7] && !strarray[8]){
        sat->setSatelliteType(g_strstrip(strarray[0]));
        sat->is_time = strncmp(strarray[1],"1",1) == 0;
        sat->active = strncmp(strarray[2],"1",1) == 0;
        sat->has_mirror = strncmp(strarray[3],"1",1) == 0;
        sat->hidden = strncmp(strarray[4],"1",1) == 0;
        double amount,angle;
        float stepsTmp;
        sp_svg_number_read_d(strarray[5], &amount);
        sp_svg_number_read_d(strarray[6], &angle);
        sp_svg_number_read_f(strarray[7], &stepsTmp);
        unsigned int steps = (unsigned int)stepsTmp;
        sat->amount = amount;
        sat->angle = angle;
        sat->steps = steps;
        g_strfreev (strarray);
        return 1;
    }
    g_strfreev (strarray);
    return 0;
}

template <>
double
ArrayParam<double>::readsvg(const gchar * str)
{
    double newx = Geom::infinity();
    sp_svg_number_read_d(str, &newx);
    return newx;
}

template <>
float
ArrayParam<float>::readsvg(const gchar * str)
{
    float newx = Geom::infinity();
    sp_svg_number_read_f(str, &newx);
    return newx;
}

template <>
Geom::Point
ArrayParam<Geom::Point>::readsvg(const gchar * str)
{
    gchar ** strarray = g_strsplit(str, ",", 2);
    double newx, newy;
    unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
    success += sp_svg_number_read_d(strarray[1], &newy);
    g_strfreev (strarray);
    if (success == 2) {
        return Geom::Point(newx, newy);
    }
    return Geom::Point(Geom::infinity(),Geom::infinity());
}

template <>
Satellite
ArrayParam<Satellite >::readsvg(const gchar * str)
{
    Satellite sat;
    if (sp_svg_satellite_read_d(str, &sat)) {
        return sat;
    }
    Satellite satellite(FILLET);
    satellite.setIsTime(true);
    satellite.setActive(false);
    satellite.setHasMirror(false);
    satellite.setHidden(true);
    satellite.setAmount(0.0);
    satellite.setAngle(0.0);
    satellite.setSteps(0);
    return satellite;
}

} /* namespace LivePathEffect */

} /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
