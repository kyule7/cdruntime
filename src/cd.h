/*
Copyright 2014, The University of Texas at Austin 
All rights reserved.

THIS FILE IS PART OF THE CONTAINMENT DOMAINS RUNTIME LIBRARY

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

// C++ interface
#include "cd_features.h"
#include "cd_def_common.h"
#ifdef __CUDACC__
  #ifndef _CPP_PROG
    #include "cd_capi.h"
  #else
    #include "cd_handle.h"
    #include "tuned_cd_handle.h"
  #endif

#else // non-cuda program

  #ifdef __cplusplus
    #include "cd_handle.h"
    #include "tuned_cd_handle.h"
  #else
    #include "cd_capi.h"
  #endif

#endif


//#if CD_MPI_ENABLED == 0 && CD_AUTOMATED == 1
//CDInitiator cd_init;
////CDPath cd_path;
////CDPath *CDPath::uniquePath_ = &cd_path;
//#endif
// C interface

/**
 * @mainpage Containment Domains 
 *
 * \author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * \date April 2015
 *
 * \note Print version available at <http://lph.ece.utexas.edu/public/CDs>
 *
 * 
 * \brief Containment Domains (CD) API v0.2 (C++) is a programming construct 
 * that allows users to express resilience needs in their program.
 * The purpose of this document is to describe a CD API to program CD-enabled applications.
 * Current CD runtime is designed for MPI programs, which also works well with signle node, single thread programs.
 *
 * A complete discussion of the semantics of Containment Domains is out of scope of this document.
 * The latest version of the semantics is available at
 * <http://lph.ece.utexas.edu/public/CDs>.
 *
 * @section sect_intro Containment Domains Overview
 * 
 * \image html cd_runtime.png "" width=10px
 *
 * Containment Domains (CDs) is a library-level approach dealing with low-overhead resilient and scalable execution. 
 * CDs abandon the prevailing one-size-fits-all approach to resilience and 
 * instead embrace the diversity of application needs, resilience mechanisms, and 
 * the deep hierarchies expected in exascale hardware and software. 
 * CDs give software a means to express resilience concerns intuitively and concisely. 
 * With CDs, software can preserve and restore state in an optimal way within the storage hierarchy 
 * and can efficiently support not globally-coordinated recovery. 
 * In addition, CDs allow software to tailor error detection, elision (ignoring some errors), and
 * recovery mechanisms to algorithmic and system needs.
 *
 *
 * \tableofcontents
 *
 * \n\n\n\n\n\n\n\n\n\n
 *
 * The supported features are descripted as below. Please take a look at it before using CD runtime system for your application.
 * - \ref sec_plans
 *
 * \n
 *
 * This version of CD runtime system works with MPICH. There are some issues with OpenMPI regarding runtime logging, which we do not resolve, yet.
 * So, please use MPICH to use current version of CD runtime system.
 * For now, the documentation is organized around the following "modules", 
 * which can also be accessed through the "Modules" tab on the HTML docs.
 *
 * - \ref cd_init_funcs
 * - \ref cd_defs
 * - \ref cd_accessor_funcs
 * - \ref cd_hierarchy
 * - \ref preservation_funcs
 * - \ref cd_detection
 *   - \ref error_reporting
 *   - \ref internal_recovery 
 *
 * - \ref runtime_logging
 * - \ref cd_event_funcs
 * - \ref cd_error_probability
 * - \ref error_injector
 *   - \ref sec_example_error_injection
 * - \ref PGAS_funcs
 *
 * - \ref sec_example_spmv
 * - \ref sec_example_lulesh
 *
 * \note  This research was, in part, funded by the U.S. Government
 *  with partial support from the Department of Energy under Awards
 *  DE-SC0008671 and DE-SC0008111. The views and conclusions contained
 *  in this document are those of the authors and should not be
 *  interpreted as representing the official policies, either
 *  expressed or implied, of the U.S. Government. 
 *
 * \example spmv.cc
 * \example lulesh.cc
 * \example test_cd_hierarchy.cc
 */

/** \page plans Road Map of CD Runtime
 *
 * \section sec_plans Supported features in CD runtime
 * \subsection supported Supported features
 *
 * \include plans.txt
 *
 */

/** \page examples_page_spmv CD Example Page 1 (SpMV)
 *
 * \section sec_example_spmv CD Example 1 - SpMV
 * \subsection example_spmv Sparse Matrix Vector Multiplication
 *
 * \include examples_for_doc/spmv.cc
 *
 */

/** \page examples_page_lulesh CD Example Page 2 (LULESH)
 *
 * \section sec_example_lulesh CD Example 2 - LULESH
 * \subsection example_lulesh Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics
 *
 * \include examples_for_doc/lulesh.cc
 *
 */
 
/** \page examples_page_test_hierarchy CD Example Page 3 (Test CD Hierarchy)
 *
 * \section sec_example_test_hierarchy CD Example 3 - CD Test Program : CD Hierarchy
 * \subsection example_test_hierarchy Test CD Hierarchy
 *
 * \include examples_for_doc/test_cd_hierarchy.cc
 *
 * \subsection example_user_defined_hierarchy User-defined CD Hierarchy Example
 *
 * \include examples_for_doc/test_user_defined_hierarchy.cc
 */

/** \page examples_page_error_injection Error Injection
 *
 * \section sec_example_error_injection Error Injection Interface
 *
 * \subsection example_error_injection User-defined Error Injector Example
 * \include examples_for_doc/example_error_injector.h 
 *
 */

// \subsection example_error_injection Description File for Error Injection
// \include examples_for_doc/error_injection_description.h

/**
 * @defgroup cd_handle CD handle
 * @ingroup user_interfaces
 * @brief Interface object which users will interact with CD runtime.
 *
 *   Following lists are the modules in CD handle class.
 *   - @ref cd_hierarchy \n
 *   - @ref preservation_funcs \n
 *   - @ref cd_detection \n
 *   - @ref cd_accessor \n
 *
 * @defgroup cd_hierarchy CD hierarchy
 * @brief Methods regarding how to create CD hierarchy in application.
 *
 * 
 * @defgroup preservation_funcs Preservation/Restoration
 * @brief These modules are regarding how to preserve and restore data in CDs.
 *
 * 
 * @defgroup cd_detection Error detection
 * @brief Error detection mechanism supported by CD runtime.
 *
 * @defgroup cd_accessor CD accessors
 * @brief The accessors of CDHandle which might be useful.
 * 
 * @defgroup cd_init_funcs CD Init Functions
 * @brief Initialization and finalization for CD runtime.
 * 
 * @defgroup cd_accessor_funcs Global CD accessor
 * @brief The \ref cd_accessor_funcs are used to get the handle for current CD or root CD.
 *
 * 
 * 
 * @defgroup cd_defs CD types/definitions.
 * @brief CD-related type definitions. 
 *
 * The \ref cd_defs module includes general CD-related type
 * definitions that don't fall into other type definitions (\ref
 * error_reporting and \ref preservation_funcs).
 *
 * 
 * @defgroup user_interfaces CD-user interfaces
 * @brief Interfaces that user need to know.
 * 
 *   @defgroup error_injector Error injection interface
 *   @brief Interface for error injection through CD runtime.
 *
 *    User can inject errors using this error injector interface. 
 *   `ErrorInjector` class is the base class that users will inherit if they want to plug in their own error injector.
 *    This functionality can be turned on/off by setting compile flag, "ENABLE_ERROR_INJECTOR", as 1.
 *    It will be helpful to take a look at how to interact with CDs to inject errors through CD runtime.
 *    - \ref sec_example_error_injection
 *   @ingroup user_interfaces
 * 
 *   @defgroup register_detection_recovery Register error detection/recovery
 *   @brief Methods to register error detecting/recovry handler.
 *   @ingroup user_interfaces
 *   
 *   @defgroup cd_split CD split interface
 *   @brief Method to split CDs to children CDs.
 *   @ingroup user_interfaces
 * 
 * 
 *   @defgroup cd_error_probability Tunning resiliency
 *   @brief Methods for Interacting with the CD Framework and Tuner
 *   @ingroup user_interfaces
 * 
 * 
 * @defgroup profiler-related Profiler-related
 * @brief Methods and types regarding CD profiler.
 * 
 * 
 * @defgroup internal_error_types Internal Error Types 
 * @brief Error types used internally in CD runtime.
 * 
 * @defgroup error_reporting Error reporting
 * @brief Error report part in CD runtime.
 *
 * 
 * @defgroup runtime_logging Runtime Logging
 * @brief Runtime logging-related functionality.
 *
 *        The \ref runtime_logging is supported in current CD runtime.
 * 
 * 
 * @defgroup PGAS_funcs PGAS-specific
 * @brief PGAS-related methods and types.
 *
 *
 * @defgroup cd_event_funcs CD event
 * @brief CD Event Functions for Non-Blocking Calls
 *
 * @defgroup internal_recovery Internal routines for recovery
 * @brief Internal Functions for Customizable Recovery
 */
