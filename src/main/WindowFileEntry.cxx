// C++
#include <map>
#include <sstream>
#include <string>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/string.hxx>

// libxpp
#include <xpp/Property.hxx>
#include <xpp/XWindowAttrs.hxx>

// xwmfs
#include "main/Exception.hxx"
#include "main/logger.hxx"
#include "main/WindowFileEntry.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

using WriteMemberFunction = void (WindowFileEntry::*)(const char*, const size_t);
using WriteMemberFunctionMap = std::map<std::string, WriteMemberFunction>;

const WriteMemberFunctionMap write_member_function_map = {
	{ "name", &WindowFileEntry::writeName },
	{ "desktop", &WindowFileEntry::writeDesktop },
	{ "control", &WindowFileEntry::writeCommand },
	{ "geometry", &WindowFileEntry::writeGeometry },
	{ "properties", &WindowFileEntry::writeProperties }
};

void WindowFileEntry::writeProperties(const char *data, const size_t bytes) {
	cosmos::MutexGuard g{Xwmfs::getInstance().getEventLock()};
	std::string input{data, bytes};

	if (input.starts_with("!")) {
		return delProperty(input.substr(1));
	} else {
		return setProperty(input);
	}
}

void WindowFileEntry::delProperty(const std::string &name) {
	m_win.delProperty(cosmos::stripped(name));
}

void WindowFileEntry::setProperty(const std::string &input) {
	const auto open_par = input.find('(');
	const auto close_par = input.find(')', open_par + 1);
	const auto assign = input.find('=', close_par + 1);

	if (open_par == input.npos || close_par == input.npos ||
			assign == input.npos || assign != close_par + 1) {
		throw Exception{"invalid syntax, expected PROP_NAME(TYPE)=VALUE"};
	}

	const auto prop_name = input.substr(0, open_par);
	const auto type_name = input.substr(open_par+1,
			close_par - open_par - 1);
	const auto value = cosmos::stripped(input.substr(assign+1));

	if (prop_name.empty() || type_name.empty() || value.empty()) {
		throw Exception{"empty argument encountered"};
	}

	if (type_name == "STRING") {
		xpp::Property<const char*> prop;
		prop = value.c_str();
		m_win.setProperty(prop_name, prop);
	} else if (type_name == "CARDINAL") {
		int parsed_int = 0;
		try {
			size_t last_parsed;
			/*
			 * with base 0 we can support the usual hexadecimal
			 * and octal syntax as well.
			 */
			parsed_int = std::stoi(value, &last_parsed, 0);

			if (last_parsed != value.size()) {
				throw Exception{"excess data in string input"};
			}
		} catch (const std::exception &ex) {
			throw Exception{cosmos::sprintf(
					"bad integer value for CARDINAL property: %s", ex.what())};
		}
		xpp::Property<int> prop;
		prop = parsed_int;
		m_win.setProperty(prop_name, prop);
	} else if (type_name == "UTF8_STRING") {
		xpp::utf8_string string_u8;
		string_u8.str = value;
		xpp::Property<xpp::utf8_string> prop;
		prop = string_u8;
		m_win.setProperty(prop_name, prop);
	} else {
		throw Exception{"unsupported property type encountered"};
	}
}

void WindowFileEntry::writeGeometry(const char *data, const size_t bytes) {
	std::stringstream ss;
	ss.str(std::string{data, bytes});

	char c = '\0';
	xpp::XWindowAttrs attrs;
	bool good = true;

	ss >> attrs.x;
	good = good && ss.good();
	ss >> c;
	good = good && c == ',';
	ss >> attrs.y;
	good = good && ss.good();
	ss >> c;
	good = good && c == ':';
	ss >> attrs.width;
	good = good && ss.good();
	ss >> c;
	good = good && c == 'x';
	ss >> attrs.height;
	good = good && ss.good();

	if (!good) {
		throw Exception{"Couldn't parse new geometry"};
	}

	m_win.moveResize(attrs);
	// send the move request out immediately
	Xwmfs::getInstance().getDisplay().flush();
}

void WindowFileEntry::writeCommand(const char *data, const size_t bytes) {
	const auto command = cosmos::to_lower(cosmos::stripped(
				std::string{data, bytes}));

	if (command == "destroy") {
		m_win.destroy();
	} else if (command == "delete") {
		m_win.sendDeleteRequest();
	} else {
		throw Exception{"invalid command encountered"};
	}
}

Entry::Bytes WindowFileEntry::write(OpenContext *ctx,
		const char *data, const size_t bytes, off_t offset) {
	(void)ctx;
	if (!m_writable) {
		throw cosmos::Errno::BAD_FD;
	}
	// we don't support writing at offsets
	if (offset) {
		throw cosmos::Errno::OP_NOT_SUPPORTED;
	}

	try {
		auto it = write_member_function_map.find(m_name);

		if (it == write_member_function_map.end()) {
			logger->error()
				<< __FUNCTION__ << ": Write call for window file entry of unknown type: \""
				<< this->m_name
				<< "\"\n";
			throw cosmos::Errno::NXIO;
		}

		auto mem_fn = it->second;

		cosmos::MutexGuard g{m_parent->getLock()};

		(this->*(mem_fn))(data, bytes);
	} catch (const std::exception &e) {
		logger->error()
			<< __FUNCTION__
			<< ": Error operating on window (node '" << this->m_name << "'): "
			<< e.what() << std::endl;
		throw cosmos::Errno::INVALID_ARG;
	}

	return Bytes{static_cast<int>(bytes)};
}

void WindowFileEntry::writeDesktop(const char *data, const size_t bytes) {
	int the_num;
	try {
		parseInteger(data, bytes, the_num);
	} catch (const cosmos::Errno err) {
		logger->error()
			<< "Failed to parse desktop number: " << err << "\n";
		throw;
	}

	Xwmfs::getInstance().getRootWin().moveToDesktop(m_win, the_num);
}

} // end ns
