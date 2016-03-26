/**
 * basiclogger.h - declarations for basic logging functionality
 *
 * $Revision: 688 $ $Author: heiko $
 *
 * (c) 2012-2013 Heiko Schäfer <heiko@rangun.de>
 *
 * LICENCE is inherited by project using this file
 *
 **/

#ifndef COMMONS_BASICLOGGER_H
#define COMMONS_BASICLOGGER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sstream>
#include <typeinfo>
#include <exception>

#include "linkercontrol.h"

#ifndef LOGGER_NULL
#define LOGGER_NULL "(null)"
#endif

#ifndef LOGGER_PREFIX
#define LOGGER_PREFIX ""
#endif

#ifndef LOGGER_CLASS
#define LOGGER_CLASS Commons::BasicLogger
#endif

#ifndef NDEBUG
#define logDebug(msg)	{ (LOGGER_CLASS(LOGGER_CLASS::LOG_DEBUG))   << LOGGER_PREFIX << msg; }
#else
#define logDebug(msg)
#endif
#define log(msg)		{ (LOGGER_CLASS(LOGGER_CLASS::LOG_NONE))    << LOGGER_PREFIX << msg; }
#define logInfo(msg)	{ (LOGGER_CLASS(LOGGER_CLASS::LOG_INFO))    << LOGGER_PREFIX << msg; }
#define logWarning(msg)	{ (LOGGER_CLASS(LOGGER_CLASS::LOG_WARNING)) << LOGGER_PREFIX << msg; }
#define logError(msg)	{ (LOGGER_CLASS(LOGGER_CLASS::LOG_ERROR))   << LOGGER_PREFIX << msg; }
#define logFatal(msg)	{ (LOGGER_CLASS(LOGGER_CLASS::LOG_FATAL))   << LOGGER_PREFIX << msg; }

namespace Commons {

class BasicLogger {
	DISALLOW_COPY_AND_ASSIGN(BasicLogger)
public:

	typedef enum _PACKED {
		LOG_NONE,
		LOG_DEBUG,
		LOG_INFO,
		LOG_WARNING,
		LOG_ERROR,
		LOG_FATAL
	} LEVEL;

	BasicLogger(const LEVEL &level);
	virtual ~BasicLogger();

	struct width {
		inline explicit width(int w) : m_width(w) {}
		int m_width;
	};

	struct nonl {};

	struct abool {
		inline abool(bool b) : b(b) {}
		bool b;
	};

	static const std::ios_base::fmtflags hex;
	static const std::ios_base::fmtflags dec;

	BasicLogger &operator<<(const std::ios_base::fmtflags &f);
	BasicLogger &operator<<(const BasicLogger::nonl &nonl);
	BasicLogger &operator<<(const BasicLogger::width &w);

	BasicLogger &operator<<(const std::wstring &msg);
	BasicLogger &operator<<(const std::string &msg);
	BasicLogger &operator<<(const std::exception &e);
	BasicLogger &operator<<(const std::type_info &ti);

	BasicLogger &operator<<(float f);
	BasicLogger &operator<<(int i);
	BasicLogger &operator<<(size_t st);
	BasicLogger &operator<<(const std::string *msg);
	BasicLogger &operator<<(const abool &b);
	BasicLogger &operator<<(const char *s);
	BasicLogger &operator<<(const wchar_t *s);

protected:
	LEVEL getLevel() const;
	_LOCAL std::wostringstream &getMessageStream();

	_LOCAL virtual void initMessageString();

private:
	_LOCAL static const std::string demangle(const char *name);

private:
	std::wostringstream m_msg;
	bool m_noNewline;
	LEVEL m_level;
};

}

#endif // COMMONS_BASICLOGGER_H

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
