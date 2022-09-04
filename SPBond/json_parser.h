#pragma once

#include <vector>
#include <string>
#include <map>

// Primitive JSON parser
class simple_json_parser;
struct object_t;
struct array_t;
struct value_t;

struct array_t
{
	std::vector<value_t> values;
};

struct object_t
{
	std::map<std::string, value_t> values;

	static const value_t null_value;
	static const std::string null_string;

	const value_t& get_value(const std::string& str) const;
	const std::string& get_string(const std::string& str) const;
};

// TODO - use std::variant
struct value_t
{
	std::string str;
	object_t obj;
	array_t array;
};

class simple_json_parser
{
public:
	const std::string& str;
	object_t& obj;
	size_t index = 0;

	simple_json_parser(const std::string& str, object_t& o);

private:
	std::map<std::string, value_t> parse_values();
	object_t parse_object();
	array_t parse_array();

	bool parse();
	char getch();
	char peek();
	void expect(char ch);
	std::string expect_string();
	value_t expect_value();
};
