// xwmfs
#include "main/WindowFileEntry.hxx"
#include "main/StdLogger.hxx"
#include "x11/XWindowAttrs.hxx"
#include "common/Helper.hxx"
#include "fuse/DirEntry.hxx"

namespace xwmfs
{

const WindowFileEntry::WriteMemberFunctionMap
	WindowFileEntry::m_write_member_function_map =
{
	{ "name", &WindowFileEntry::writeName },
	{ "desktop", &WindowFileEntry::writeDesktop },
	{ "control", &WindowFileEntry::writeCommand },
	{ "geometry", &WindowFileEntry::writeGeometry },
	{ "properties", &WindowFileEntry::writeProperties }
};

void WindowFileEntry::writeProperties(const char *data, const size_t bytes)
{
	std::string input(data, bytes);

	if( !input.empty() && input[0] == '!' )
	{
		return delProperty(input.substr(1));
	}
	else
	{
		return setProperty(input);
	}
}

void WindowFileEntry::delProperty(const std::string &name)
{
	m_win.delProperty(stripped(name));
}

void WindowFileEntry::setProperty(const std::string &input)
{
	const auto open_par = input.find('(');
	const auto close_par = input.find(')', open_par + 1);
	const auto assign = input.find('=', close_par + 1);

	if(
		open_par == input.npos ||
		close_par == input.npos ||
		assign == input.npos ||
		assign != close_par + 1
	)
	{
		xwmfs_throw(Exception("invalid syntax, expected PROP_NAME(TYPE)=VALUE"));
	}

	const auto prop_name = input.substr(0, open_par);
	const auto type_name = input.substr(open_par+1, close_par - open_par - 1);
	const auto value = stripped(input.substr(assign+1));

	if( prop_name.empty() || type_name.empty() || value.empty() )
	{
		xwmfs_throw(Exception("empty argument encountered"));
	}

	if( type_name == "STRING" )
	{
		Property<const char*> prop;
		prop = value.c_str();
		m_win.setProperty(prop_name, prop);
	}
	else if( type_name == "CARDINAL" )
	{
		std::stringstream ss;
		int int_value;
		ss.str(value);
		ss >> int_value;
		if( ss.fail() )
		{
			// NOTE: this parsing is not completely bullet proof
			// against extra non-numeric characters or hex based
			// numbers yet
			xwmfs_throw(Exception("non-integer value for CARDINAL property"));
		}
		Property<int> prop;
		prop = int_value;
		m_win.setProperty(prop_name, prop);
	}
	else if( type_name == "UTF8_STRING" )
	{
		utf8_string string_u8;
		string_u8.data = value.c_str();
		Property<utf8_string> prop;
		prop = string_u8;
		m_win.setProperty(prop_name, prop);
	}
	else
	{
		xwmfs_throw(Exception("unsupported property type encountered"));
	}
}

void WindowFileEntry::writeGeometry(const char *data, const size_t bytes)
{
	std::stringstream ss;
	ss.str(std::string(data, bytes));

	char c = '\0';
	XWindowAttrs attrs;
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

	if( !good )
	{
		xwmfs_throw(
			xwmfs::Exception("Couldn't parse new geometry")
		);
	}

	m_win.moveResize(attrs);
	// send the move request out immediately
	XDisplay::getInstance().flush();
}

void WindowFileEntry::writeCommand(const char *data, const size_t bytes)
{
	const auto command = tolower(stripped(std::string(data, bytes)));

	if( command == "destroy" )
	{
		m_win.destroy();
	}
	else if( command == "delete" )
	{
		m_win.sendDeleteRequest();
	}
	else
	{
		xwmfs_throw(
			xwmfs::Exception("invalid command encountered")
		);
	}
}

int WindowFileEntry::write(OpenContext *ctx, const char *data, const size_t bytes, off_t offset)
{
	(void)ctx;
	if( ! m_writable )
		return -EBADF;
	// we don't support writing at offsets
	if( offset )
		return -EOPNOTSUPP;

	try
	{
		auto it = m_write_member_function_map.find(m_name);

		if( it == m_write_member_function_map.end() )
		{
			xwmfs::StdLogger::getInstance().error()
				<< __FUNCTION__
				<< ": Write call for window file entry of unknown type: \""
				<< this->m_name
				<< "\"\n";
			return -ENXIO;
		}

		auto mem_fn = it->second;

		MutexGuard g(m_parent->getLock());

		(this->*(mem_fn))(data, bytes);
	}
	catch( const xwmfs::Exception &e )
	{
		xwmfs::StdLogger::getInstance().error()
			<< __FUNCTION__
			<< ": Error operating on window (node '"
			<< this->m_name << "'): "
			<< e.what() << std::endl;
		return -EINVAL;
	}

	return bytes;
}

} // end ns

