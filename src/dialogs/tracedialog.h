#ifndef __TRACEDIALOG_H__
#define __TRACEDIALOG_H__
/*
 * A simple dialog for setting the parameters for autotracing a
 * bitmap <image> into an svg <path>
 *
 * Authors:
 *   Bob Jamison
 *   Other dudes from The Inkscape Organization
 *
 * Copyright (C) 2004 The Inkscape Organization
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

namespace Inkscape {
namespace UI {
namespace Dialogs {


/**
 * A dialog that displays log messages
 */
class TraceDialog
{

public:
    

    /**
     * Constructor
     */
    TraceDialog() {  };


    /**
     * Factory method
     */
    static TraceDialog *create();

    /**
     * Destructor
     */
    virtual ~TraceDialog() {};


    /**
     * Show the dialog
     */
    virtual void show() = 0;

    /**
     * Do not show the dialog
     */
    virtual void hide() = 0;

    /**
     * Get a shared singleton instance
     */
    static TraceDialog *getInstance();

    /**
     * Show the instance above
     */
    static void showInstance();

};


} //namespace Dialogs
} //namespace UI
} //namespace Inkscape




#endif /* __TRACEDIALOG_H__ */

