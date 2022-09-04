#include "pch.h"
#include "json_parser.h"

using namespace std;

const value_t& object_t::get_value(const string& str) const
{
	auto iter = values.find(str);
	if (iter == values.end())
		return null_value;
	return iter->second;
}

const string& object_t::get_string(const string& str) const
{
	auto iter = values.find(str);
	if (iter == values.end())
		return null_string;
	return iter->second.str;
}

const value_t object_t::null_value;
const string object_t::null_string;

simple_json_parser::simple_json_parser(const string& str, object_t& o) : str(str), obj(o)
{
	parse();
}

map<string, value_t> simple_json_parser::parse_values()
{
	map<string, value_t> values;
	while (1)
	{
		auto name = expect_string();
		expect(':');
		if (peek() == '{')
		{
			value_t v;
			v.obj = parse_object();
			values[name] = std::move(v);
		}
		else if (peek() == '[')
		{
			// Parse array
			value_t v;
			v.array = parse_array();
			values[name] = std::move(v);
		}
		else
			values[name] = expect_value();
		auto ch = peek();
		switch (ch)
		{
		case '}':
			return values;
		case ',':
			getch();
			break;
		default:
			throw ch;
		}
	}
}

object_t simple_json_parser::parse_object()
{
	object_t o;
	expect('{');
	o.values = parse_values();
	expect('}');
	return o;
}

array_t simple_json_parser::parse_array()
{
	array_t o;
	expect('[');
	while (1)
	{
		value_t v;
		v.str = expect_string();
		o.values.push_back(std::move(v));
		if (peek() != ',')
			break;
		expect(',');
	}
	expect(']');
	return o;
}

bool simple_json_parser::parse()
{
	try
	{
		obj = parse_object();
	}
	catch (char)
	{
		return false;
	}
	return true;
}

char simple_json_parser::getch()
{
	_ASSERTE(index < str.size());
	if (index >= str.size())
		throw 0;
	return str[index++];
}

char simple_json_parser::peek()
{
	_ASSERTE(index < str.size());
	if (index >= str.size())
		throw 0;
	return str[index];
}

void simple_json_parser::expect(char ch)
{
	auto c = getch();
	if (c != ch)
		throw c;
}

string simple_json_parser::expect_string()
{
	char ch;
	expect('"');
	string str;
	while ((ch = getch()) != '"')
		str += ch;
	return str;
}

value_t simple_json_parser::expect_value()
{
	value_t v;
	// TODO - use variant
	if (peek() == '"')
		v.str = expect_string();
	else
	{
		// Parse to ,
		string str;
		while ((peek() != ',') && (peek() != '}'))
			str += getch();
	}
	return v;
}
