/**
 * linkercontrol.h - a common header for controlling visibility of functions and methods
 *
 * $Revision: 673 $ $Author: heiko $
 *
 * (c) 2012-2013 Heiko Schäfer <heiko@rangun.de>
 *
 * LICENCE is inherited by project using this file
 *
 **/

#ifndef COMMONS_LINKERCONTROL_H
#define COMMONS_LINKERCONTROL_H

namespace Commons {

// Generic helper definitions for shared library support
#if (defined _WIN32 || defined __CYGWIN__) && !defined STATIC_BUILD
#define _IMPORT __declspec(dllimport)
#define _EXPORT __declspec(dllexport)
#define _LOCAL
#else
#if __GNUC__ >= 4 & !defined STATIC_BUILD
#define _IMPORT __attribute__ ((visibility ("default")))
#define _EXPORT __attribute__ ((visibility ("default")))
#define _LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define _IMPORT
#define _EXPORT
#define _LOCAL
#endif
#endif

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	_LOCAL TypeName(const TypeName&); \
	_LOCAL TypeName& operator=(const TypeName&);

#define FINALBASE(P) \
	template<class C> \
	class _EXPORT __final##P { \
		_LOCAL __final##P(const __final##P&); \
		_LOCAL __final##P& operator=(const __final##P&); \
		_LOCAL __final##P() {} \
		_LOCAL virtual ~__final##P() {} \
		friend class P; \
	};

#define FINALCC(P) __final##P<P>()

#define FINAL(P) public virtual __final##P<P>

#define _PACKED __attribute__ ((__packed__))

}

#endif /* COMMONS_LINKERCONTROL_H */

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
