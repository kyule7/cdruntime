#include "cd_handle.h"
#include "cd_path.h"


/**
 * @mainpage Containment Domains 
 *
 * \author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * \date March 2015
 *
 * \note Print version available at <http://lph.ece.utexas.edu/public/CDs>
 *
 * 
 * \brief Containment Domains API v0.2 (C++)
 * The purpose of this document is to describe a CD API for
 * programming CD-enabled applications. A complete discussion of the
 * semantics of containment domains is out of scope of this document; the
 * latest version of the semantics is available at
 * <http://lph.ece.utexas.edu/public/CDs>.
 *
 * @section sect_intro Containment Domains Overview
 * 
 * Containment domains (CDs) are a new approach that achieves
 * low-overhead resilient and scalable
 * execution (see http://lph.ece.utexas.edu/public/CDs). CDs abandon the prevailing
 * one-size-fits-all approach to resilience and instead embrace the
 * diversity of application needs, resilience mechanisms, and the deep
 * hierarchies expected in exascale hardware and software. CDs give
 * software a means to express resilience concerns intuitively and
 * concisely. With CDs, software can preserve and restore state in an
 * optimal way within the storage hierarchy and can efficiently
 * support uncoordinated recovery. In addition, CDs allow software to
 * tailor error detection, elision (ignoring some errors), and
 * recovery mechanisms to algorithmic and system needs.
 *
 * \tableofcontents
 *
 * \todo Write a more significant introduction and put sections for
 * what are currently modules
 *
 * For now, the documentation is organized around the following
 * "modules", which can also be accessed through the "Modules" tab on
 * the HTML docs.
 *
 * - \ref cd_init_funcs
 * - \ref cd_accessor_funcs
 * - \ref cd_hierarchy
 * - \ref preservation_funcs
 * - \ref detection_recovery
 *   - \ref error_reporting
 *   - \ref internal_recovery 
 *   
 * - \ref cd_error_probability
 * - \ref PGAS_funcs
 * - \ref cd_event_funcs
 * - \ref cd_defs
 *
 * - \ref sec_examples
 *
 * \note  This research was, in part, funded by the U.S. Government
 *  with partial support from the Department of Energy under Awards
 *  DE-SC0008671 and DE-SC0008111. The views and conclusions contained
 *  in this document are those of the authors and should not be
 *  interpreted as representing the official policies, either
 *  expressed or implied, of the U.S. Government. 
 *
 * \example spmv.cc
 */

/** \page examples_page Examples
 *
 * \section sec_examples Examples
 * \subsection example_spmv SpMV Example
 *
 * \include examples/spmv.cc
 *
 */


