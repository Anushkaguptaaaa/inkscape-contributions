#ifndef __ECMA_BINDING_H__
#define __ECMA_BINDING_H__

/*
 * Provide the binding of ECMAScript to the SVG Tree
 *
 * See the README file in this directory for more information.
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *   Others
 *
 * Copyright (C) 2004 Inkscape.org
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <inkscape.h>

#include <exception>

namespace Inkscape {


//Forward declarations
class  EcmaObject;
struct EcmaObjectPrivate;  //hide impl specifics
class  EcmaBinding;
struct EcmaBindingPrivate;  //hide impl specifics



/**
 * This Exception class can allow us to customize the messages generated
 * by anomalies in the binding and engine.
 */
class EcmaException : public std::exception
{
    public:
        EcmaException(const char *theReason) { reason=(char *)theReason; };
	const char *what() const throw() { return reason; }
    private:
    	char *reason;

};




/**
 * This is a one-for-one mapping of every node of the SVG tree
 */
class EcmaObject
{



public:
    /**
     * Constructor.
     *
     * @param owner.  The EcmaBinding container that owns this object
     * @param parent  The node above this one in the tree
     * chunk.
     *
     */
    EcmaObject(EcmaBinding *owner, EcmaObject *parent) throw (EcmaException);

    /**
     * Destructor.  Should perform any cleanup, esp the JSContext
     * library.
     */
    virtual ~EcmaObject();

    /**
     * Add a child object to this object
     * @param newNode node to add to this object
     */
    void EcmaObject::addChild(EcmaObject *newNode);


    /**
     * Implementation-specific data
     */
    struct EcmaObjectPrivate *pdata;


    /**
     * My owner
     */
    EcmaBinding *owner;


protected:

    /**
     * Trace messages
     */
    void trace(char *fmt, ...);

    /**
     * Error messages
     */
    void error(char *fmt, ...);


private:

    /**
     * The node above me
     */
    EcmaObject  *parent;
    
    /**
     * Any nodes that I own
     */
    EcmaObject  *children;
    
    /**
     * My next sibling node under the same parent
     */
    EcmaObject  *next;




};//class EcmaObject


















/**
 *
 * This is the container of the JSENgine and the EcmaScript nodes
 * in an SVG tree.
 *
 */
class EcmaBinding
{


public:

    /**
     * Constructor.
     *
     * @param parent.  This is the Inkscape application that owns this
     * EcmaBinding.
     *
     */
    EcmaBinding(Inkscape::Application *parent) throw (EcmaException);

    /**
     * Destructor.  Should perform any cleanup, esp in the Spidermonkey
     * library.
     */
    virtual ~EcmaBinding();
    
    /** 
     * Recursive processing of the internal SVG tree.  Called by
     * processDocument() and itself.
     *
     * @param node.  The current node to process, with its children
     * @return  true for success..  false for a 'soft' error.  Throw
     * an exception for a 'hard' error.
     *
     */
    EcmaObject *EcmaBinding::processNode(Inkscape::XML::Node *node, EcmaObject *parent)
                                                throw (EcmaException);



    /**
     * Get ECMAScript nodes from document and compile scripts
     * This is before running anything.
     *
     * @param document.  The SVG document to process.
     *
     */
    bool processDocument(SPDocument *document) throw (EcmaException);

    /**
     * Root object of our EcmaObject tree
     */
    EcmaObject *rootObject;

    /**
     * Implementation-specific data
     */
    struct EcmaBindingPrivate *pdata;
    
    /**
     * Test binding from the application
     *
     */
    static int testMe();


protected:

    /**
     * Trace messages
     */
    void trace(char *fmt, ...);

    /**
     * Error messages
     */
    void error(char *fmt, ...);


private:

    /**
     * The Inkscape application that owns this EcmaBinding engine
     */
    Inkscape::Application *parent;

    /**
     * The document to which we will bind
     */
    SPDocument            *document;

    /**
     * The REPR tree root to which we will bind
     */
    Inkscape::XML::Node                *root;




};//class EcmaBinding







}//namespace Inkscape

#endif /*__ECMA_BINDING_H__*/
