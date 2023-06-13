#include "nin-io/ainb/CommandBody.h"

using namespace std;


CommandBody::CommandBody()
{
	m_address = -1;
}

CommandBody::CommandBody(StringList* string_list, ParameterHandler* parameter_handler)
{
	m_string_list = string_list;
	m_parameter_handler = parameter_handler;
	m_address = -1;

}

CommandBody::~CommandBody()
{

}

void CommandBody::load(fstream& file)
{
	m_address = file.tellg();
	for (int i = 0; i < 6; i++) {
		int param_num;
		readIntFromStream(file, is_big_endian, 4, param_num);

		int param_value;
		readIntFromStream(file, is_big_endian, 4, param_value);

		Parameter parameter;
		parameter.section_num = i;
		parameter.index = param_num;
		parameter.value = param_value;
		//parameter.param_name = m_parameter_handler->getParameterFromTable(i, param_num).name;

		m_table_parameters.push_back(parameter);
	}

	for (int i = 0; i < 12; i++) {
		int param_num;
		readIntFromStream(file, is_big_endian, 4, param_num);

		int param_value;
		readIntFromStream(file, is_big_endian, 4, param_value);

		Parameter parameter;
		parameter.section_num = i;
		parameter.index = param_num;
		parameter.value = param_value;
		//parameter.param_name = m_parameter_handler->getParameterFromStructure(i, param_num).name;

		m_section_parameters.push_back(parameter);
	}

	// TODO: load table
	file.seekg(m_address, ios::beg);
	file.seekg(0xa3, ios::cur);

	int table_size;
	readIntFromStream(file, is_big_endian, 1, table_size);

	if (table_size == 0) {
		return;
	}

	vector<int> entry_addresses;
	for (int i = 0; i < table_size; i++) {
		int entry_address;
		readIntFromStream(file, is_big_endian, 4, entry_address);
		entry_addresses.push_back(entry_address);
	}

	for (int i = 0; i < table_size; i++) {
		file.seekg(entry_addresses[i], ios::beg);

		CallTableEntry pair;
		readIntFromStream(file, is_big_endian, 4, pair.index);

		int string_tag;
		readIntFromStream(file, is_big_endian, 4, string_tag);
		pair.parameter = m_string_list->getString(string_tag);

		m_call_table.push_back(pair);
	}
	
}

vector<CommandBody::Parameter>* CommandBody::getTableParameters()
{
	return &m_table_parameters;
}

vector<CommandBody::Parameter>* CommandBody::getStructureParameters()
{
	return &m_section_parameters;
}
