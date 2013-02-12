/*
 *
 *                This source code is part of
 *
 *                 G   R   O   M   A   C   S
 *
 *          GROningen MAchine for Chemical Simulations
 *
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2009, The GROMACS development team,
 * check out http://www.gromacs.org for more information.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 *
 * For more info, check our website at http://www.gromacs.org
 */
/*! \file
 * \brief
 * Declares common exception classes for fatal error handling.
 *
 * \author Teemu Murtola <teemu.murtola@cbr.su.se>
 * \inpublicapi
 * \ingroup module_utility
 */
#ifndef GMX_UTILITY_EXCEPTIONS_H
#define GMX_UTILITY_EXCEPTIONS_H

#include <cstdio>
#include <cstdlib>

#include <exception>
#include <string>
#include <vector>

#include <boost/exception_ptr.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>

namespace gmx
{

namespace internal
{
//! Internal container type for storing a list of nested exceptions.
typedef std::vector<boost::exception_ptr> NestedExceptionList;
}   // namespace internal

/*! \addtopublicapi
 * \{
 */

/*! \brief
 * Provides information for Gromacs exception constructors.
 *
 * This class exists to implement common functionality for initializing all
 * Gromacs exceptions without having extra code in each exception class.
 * In simple cases, it can be implicitly constructed by passing a simple string
 * to an exception constructor.
 * If more complex initialization is necessary, it is possible to explicitly
 * construct an object of this type and then call other methods to add
 * information before actually creating the exception object.
 *
 * \todo
 * With the exception of the reason string, information added with this class
 * is not currently accessible through any public API, except for calling
 * printFatalErrorMessage().  This is not implemented as there is no current
 * need for it, and it is not clear what would be the best alternative for the
 * access.  It should be possible to refactor the internal implementation to
 * suit the needs of such external access without requiring changes in code
 * that throws these exceptions.
 *
 * \ingroup module_utility
 */
class ExceptionInitializer
{
    public:
        /*! \brief
         * Creates an initialized with the given string as the reason.
         *
         * \param[in] reason  Detailed reason for the exception.
         * \throw     std::bad_alloc if out of memory.
         *
         * This constructor is not explicit to allow constructing exceptions
         * with a plain string argument given to the constructor without adding
         * extra code to each exception class.
         */
        ExceptionInitializer(const char *reason)
            : reason_(reason)
        {
        }
        //! \copydoc ExceptionInitializer(const char *)
        ExceptionInitializer(const std::string &reason)
            : reason_(reason)
        {
        }

        /*! \brief
         * Returns true if addCurrentExceptionAsNested() has been called.
         *
         * Provided for convenience for cases where exceptions will be added
         * conditionally, and the caller wants to check whether any excetions
         * were actually added.
         */
        bool hasNestedExceptions() const { return !nested_.empty(); }
        /*! \brief
         * Adds the currently caught exception as a nested exception.
         *
         * May be called multiple times; all provided exceptions will be added
         * in a list of nested exceptions.
         *
         * Must not be called outside a catch block.
         */
        void addCurrentExceptionAsNested()
        {
            nested_.push_back(boost::current_exception());
        }

    private:
        std::string                     reason_;
        internal::NestedExceptionList   nested_;

        friend class GromacsException;
};

/*! \brief
 * Base class for all exception objects in Gromacs.
 *
 * Although boost recommends using virtual inheritance in exception hiearchies,
 * it is not used here for two reasons:
 * -# It is only useful when there is diamond inheritance, and that should
 *    never occur in this exception hierarchy because this class is the only
 *    instance of multiple inheritance (Gromacs programming guidelines prohibit
 *    multiple inheritance from concrete classes, but it is unavoidable here
 *    because of the design of boost::exception).
 * -# Because the constructor takes an argument, virtual inheritance would
 *    complicate any classes that inherit indirectly from this class.
 *
 * \ingroup module_utility
 */
class GromacsException : public std::exception, public boost::exception
{
    public:
        /*! \brief
         * Returns the reason string for the exception.
         *
         * The return value is the string that was passed to the constructor.
         */
        virtual const char *what() const throw();
        /*! \brief
         * Returns the error code corresponding to the exception type.
         */
        virtual int errorCode() const = 0;

        /*! \brief
         * Adds context information to this exception.
         *
         * \param[in] context  Context string to add.
         * \throws    std::bad_alloc if out of memory.
         *
         * Typical use is to add additional information higher up in the call
         * stack using this function in a catch block and the rethrow the
         * exception.
         *
         * \todo
         * The added information is currently not accessible through what(),
         * nor through any other means except for calling
         * printFatalErrorMessage(). See ExceptionInitializer for more
         * discussion.
         */
        void prependContext(const std::string &context);

    protected:
        /*! \brief
         * Creates an exception object with the provided initializer/reason.
         *
         * \param[in] details  Initializer for the exception.
         * \throws    std::bad_alloc if out of memory.
         */
        explicit GromacsException(const ExceptionInitializer &details);
};

/*! \brief
 * Exception class for file I/O errors.
 *
 * \ingroup module_utility
 */
class FileIOError : public GromacsException
{
    public:
        /*! \brief
         * Creates an exception object with the provided initializer/reason.
         *
         * \param[in] details  Initializer for the exception.
         * \throws    std::bad_alloc if out of memory.
         *
         * It is possible to call this constructor either with an explicit
         * ExceptionInitializer object (useful for more complex cases), or
         * a simple string if only a reason string needs to be provided.
         */
        explicit FileIOError(const ExceptionInitializer &details)
            : GromacsException(details) {}

        virtual int errorCode() const;
};

/*! \brief
 * Exception class for user input errors.
 *
 * Derived classes should be used to indicate the nature of the error instead
 * of throwing this class directly.
 *
 * \ingroup module_utility
 */
class UserInputError : public GromacsException
{
    protected:
        //! \copydoc FileIOError::FileIOError()
        explicit UserInputError(const ExceptionInitializer &details)
            : GromacsException(details) {}
};

/*! \brief
 * Exception class for situations where user input cannot be parsed/understood.
 *
 * \ingroup module_utility
 */
class InvalidInputError : public UserInputError
{
    public:
        //! \copydoc FileIOError::FileIOError()
        explicit InvalidInputError(const ExceptionInitializer &details)
            : UserInputError(details) {}

        virtual int errorCode() const;
};

/*! \brief
 * Exception class for situations where user input is inconsistent.
 *
 * \ingroup module_utility
 */
class InconsistentInputError : public UserInputError
{
    public:
        //! \copydoc FileIOError::FileIOError()
        explicit InconsistentInputError(const ExceptionInitializer &details)
            : UserInputError(details) {}

        virtual int errorCode() const;
};

/*! \brief
 * Exception class for simulation instabilities.
 *
 * \ingroup module_utility
 */
class SimulationInstabilityError : public GromacsException
{
    public:
        //! \copydoc FileIOError::FileIOError()
        explicit SimulationInstabilityError(const ExceptionInitializer &details)
            : GromacsException(details) {}

        virtual int errorCode() const;
};

/*! \brief
 * Exception class for internal errors.
 *
 * \ingroup module_utility
 */
class InternalError : public GromacsException
{
    public:
        //! \copydoc FileIOError::FileIOError()
        explicit InternalError(const ExceptionInitializer &details)
            : GromacsException(details) {}

        virtual int errorCode() const;
};

/*! \brief
 * Exception class for incorrect use of an API.
 *
 * \ingroup module_utility
 */
class APIError : public GromacsException
{
    public:
        //! \copydoc FileIOError::FileIOError()
        explicit APIError(const ExceptionInitializer &details)
            : GromacsException(details) {}

        virtual int errorCode() const;
};

/*! \brief
 * Exception class for use of an unimplemented feature.
 *
 * \ingroup module_utility
 */
class NotImplementedError : public APIError
{
    public:
        //! \copydoc FileIOError::FileIOError()
        explicit NotImplementedError(const ExceptionInitializer &details)
            : APIError(details) {}

        virtual int errorCode() const;
};


/*! \brief
 * Macro for throwing an exception.
 *
 * \param[in] e    Exception object to throw.
 *
 * Using this macro instead of \c throw directly makes it possible to uniformly
 * attach information into the exception objects.
 * \p e should evaluate to an instance of an object derived from
 * GromacsException.
 *
 * Basic usage:
 * \code
   if (value < 0)
   {
       GMX_THROW(InconsistentUserInput("Negative values not allowed for value"));
   }
 * \endcode
 */
#define GMX_THROW(e) \
    BOOST_THROW_EXCEPTION((e))

/*! \brief
 * Macro for throwing an exception based on errno.
 *
 * \param[in] e       Exception object to throw.
 * \param[in] syscall Name of the syscall that returned the error.
 * \param[in] err     errno value returned by the syscall.
 *
 * This macro provides a convenience interface for throwing an exception to
 * report an error based on a errno value.  In addition to adding the necessary
 * information to the exception object, the macro also ensures that \p errno is
 * evaluated before, e.g., the constructor of \p e may call other functions
 * that could overwrite the errno value.
 * \p e should evaluate to an instance of an object derived from
 * GromacsException.
 *
 * Typical usage (note that gmx::File wraps this particular case):
 * \code
   FILE *fp = fopen("filename.txt", "r");
   if (fp == NULL)
   {
       GMX_THROW(FileIOError("Could not open file"), "fopen", errno);
   }
 * \endcode
 */
#define GMX_THROW_WITH_ERRNO(e, syscall, err) \
    do { \
        int stored_errno_ = (err); \
        GMX_THROW((e) << boost::errinfo_errno(stored_errno_) \
                  << boost::errinfo_api_function(syscall)); \
    } while (0)

/*! \brief
 * Formats a standard fatal error message for reporting an exception.
 *
 * Does not throw.  If memory allocation fails or some other error occurs
 * while formatting the error, tries to print a reasonable alternative message.
 *
 * Normal usage in Gromacs command-line programs is like this:
 * \code
   int main(int argc, char *argv[])
   {
       gmx::ProgramInfo::init(argc, argv);
       try
       {
           // The actual code for the program
           return 0;
       }
       catch (const std::exception &ex)
       {
           gmx::printFatalErrorMessage(stderr, ex);
           return 1;
       }
   }
 * \endcode
 */
void printFatalErrorMessage(FILE *fp, const std::exception &ex);

/*! \brief
 * Converts an exception into a return code.
 */
int translateException(const std::exception &ex);

/*!\}*/

/*! \cond libapi */
/*! \libinternal \brief
 * Macro for catching exceptions at C++ -> C boundary.
 *
 * This macro is intended for uniform handling of exceptions when C++ code is
 * called from C code within Gromacs.  Since most existing code is written
 * using the assumption that fatal errors terminate the program, this macro
 * implements this behavior for exceptions.  It should only be used in cases
 * where the error cannot be propagated upwards using return values or such.
 *
 * Having this as a macro instead of having the same code in each place makes
 * it easy to 1) find all such locations in the code, and 2) change the exact
 * behavior if needed.
 *
 * Usage:
 * \code
   try
   {
       // C++ code
   }
   GMX_CATCH_ALL_AND_EXIT_WITH_FATAL_ERROR;
 * \endcode
 *
 * \inlibraryapi
 */
#define GMX_CATCH_ALL_AND_EXIT_WITH_FATAL_ERROR \
    catch (const std::exception &ex) { \
        ::gmx::printFatalErrorMessage(stderr, ex); \
        std::exit(1); \
    }
//! \endcond

} // namespace gmx

#endif
