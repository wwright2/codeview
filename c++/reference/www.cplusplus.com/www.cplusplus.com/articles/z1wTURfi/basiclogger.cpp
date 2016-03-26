/**
 * basiclogger.cpp - implementation for basic logging functionality
 *
 * $Revision: 687 $ $Author: heiko $
 *
 * (c) 2012-2013 Heiko Schäfer <heiko@rangun.de>
 *
 * LICENCE is inherited by project using this file
 *
 **/

#include <iostream>
#include <sstream>
#include <cstdlib>

#include "basiclogger.h"

#ifdef HAVE_GCC_ABI_DEMANGLE
#include <cxxabi.h>
#endif

using namespace Commons;

const std::ios_base::fmtflags BasicLogger::hex = std::ios_base::hex;
const std::ios_base::fmtflags BasicLogger::dec = std::ios_base::dec;

BasicLogger::BasicLogger(const BasicLogger::LEVEL &level) : m_msg(), m_noNewline(false),
	m_level(level) {
	initMessageString();
}

BasicLogger::~BasicLogger() {
	std::wclog << m_msg.str();

	if(!m_noNewline) {
		std::wclog << std::endl;
	} else {
		std::wclog.flush();
	}
}

void BasicLogger::initMessageString() {

	switch(getLevel()) {
	case LOG_NONE:
		getMessageStream() << "";
		break;
	case LOG_INFO:
		getMessageStream() << "INFO: ";
		break;
	case LOG_WARNING:
		getMessageStream() << "WARNING: ";
		break;
	case LOG_ERROR:
		getMessageStream() << "ERROR: ";
		break;
	case LOG_FATAL:
		getMessageStream() << "FATAL: ";
		break;
	case LOG_DEBUG:
	default:
		getMessageStream() << "DEBUG: ";
		break;
	}
}

std::wostringstream &BasicLogger::getMessageStream() {
	return m_msg;
}

BasicLogger::LEVEL BasicLogger::getLevel() const {
	return m_level;
}

const std::string BasicLogger::demangle(const char *name) {
#ifdef HAVE_GCC_ABI_DEMANGLE
	int status = -4;
	char *res = abi::__cxa_demangle(name, NULL, NULL, &status);
	const char *const demangled_name = (status == 0) ? res : name;
	std::string ret_val(demangled_name);
	free(res);
	return ret_val;
#else
	return name;
#endif
}

BasicLogger &BasicLogger::operator<<(const std::ios_base::fmtflags &f) {
	getMessageStream().flags(f);
	getMessageStream() << "0x";
	return *this;
}

BasicLogger &BasicLogger::operator<<(const BasicLogger::nonl &) {
	m_noNewline = true;
	return *this;
}

BasicLogger &BasicLogger::operator<<(const BasicLogger::width &w) {
	getMessageStream().width(w.m_width);
	return *this;
}

BasicLogger &BasicLogger::operator<<(const std::exception &e) {
	getMessageStream() << e.what();
	return *this;
}

BasicLogger &BasicLogger::operator<<(const std::wstring &msg) {
	getMessageStream() << msg;
	return *this;
}

BasicLogger &BasicLogger::operator<<(const std::string &msg) {
	getMessageStream() << std::wstring(msg.begin(), msg.end());
	return *this;
}

BasicLogger &BasicLogger::operator<<(const std::type_info &ti) {
	getMessageStream() << demangle(ti.name()).c_str();
	return *this;
}

BasicLogger &BasicLogger::operator<<(const std::string *msg) {
	std::string s = (msg ? (*msg) : std::string(LOGGER_NULL));
	getMessageStream() << std::wstring(s.begin(), s.end());
	return *this;
}

BasicLogger &BasicLogger::operator<<(const char *s) {
	std::string sc(s);
	getMessageStream() << std::wstring(sc.begin(), sc.end());
	return *this;
}

BasicLogger &BasicLogger::operator<<(const wchar_t *s) {
	getMessageStream() << s;
	return *this;
}

BasicLogger &BasicLogger::operator<<(size_t st) {
	getMessageStream() << (size_t)st;
	return *this;
}

BasicLogger &BasicLogger::operator<<(const abool &b) {
	getMessageStream() << std::boolalpha << (bool)b.b;
	return *this;
}

BasicLogger &BasicLogger::operator<<(float f) {
	getMessageStream() << (float)f;
	return *this;
}

BasicLogger &BasicLogger::operator<<(int i) {
	getMessageStream() << (int)i;
	return *this;
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
