#include "nin-io/ainb/Parameters.h"
#include "nin-io/ainb/StringList.h"
#include "nin-io/ainb/ParameterNode.h"

using namespace std;

Parameters::Parameters(fstream& file, StringList* string_list)
{
	m_string_list = string_list;

	// find start of the table section
	file.seekg(0x2c, ios::beg);
	readIntFromStream(file, is_big_endian, 4, m_table_section_start);


	// find start of the structure section
	file.seekg(0x34, ios::beg);
	readIntFromStream(file, is_big_endian, 4, m_structure_section_start);

	// find start of section three
	file.seekg(0x30, ios::beg);
	readIntFromStream(file, is_big_endian, 4, m_section_3_start);

	// find start of potential function section
	file.seekg(0x5c, ios::beg);
	int section_four_start;
	readIntFromStream(file, is_big_endian, 4, section_four_start);

	// load table section
	file.seekg(m_table_section_start, ios::beg);
	m_table_section_data = loadTableSection(file);

	printTableSectionData(m_table_section_data);

	for (int i = 0; i < m_table_section_data.tables.size(); i++) {
		TableData table = m_table_section_data.tables[i];
		for (int j = 0; j < table.entries.size(); j++) {
			m_data.table.push_back(table.entries[j]);
		}
	}

	//m_data.tables = m_table_section_data.tables;

	// load structure section
	file.seekg(m_structure_section_start, ios::beg);
	m_structure_section_data = loadStructureSection(file);
	m_data.parameters = m_structure_section_data.parameter_nodes;


	return;

}

Parameters::~Parameters()
{

}

Parameters::TableSectionData Parameters::loadTableSection(fstream& file)
{
	TableSectionData table_section_data;
	
	vector<int> table_section_addresses;

	int current_pos = file.tellg();
	for (int i = 0; i < 6; i++) {
		file.seekg(current_pos, ios::beg);
		file.seekg(4*i, ios::cur);

		int entry_address;
		readIntFromStream(file, is_big_endian, 4, entry_address);
		file.seekg(entry_address, ios::beg);

		if (entry_address == m_structure_section_start) {
			break;
		}
		table_section_addresses.push_back(entry_address);
	}

	for (int i = 0; i < table_section_addresses.size(); i++) {
		TableData table_data;
		int current_pos = table_section_addresses[i];
		file.seekg(current_pos, ios::beg);
		table_data.address = current_pos;

		int end_pos;
		if (i + 1 < table_section_addresses.size()) {
			end_pos = table_section_addresses[i+1];
		}
		else {
			end_pos = m_structure_section_start;
		}

		while (current_pos < end_pos) {
			TableEntry entry;

			// get name
			int name_string_tag;
			readIntFromStream(file, is_big_endian, 4, name_string_tag);
			entry.name = m_string_list->getString(name_string_tag);

			file.seekg(0x4, ios::cur); // skip 4 bytes, assumed to be 0x00000000
			int var_string_tag;
			readIntFromStream(file, is_big_endian, 4, var_string_tag);
			entry.var = to_string(var_string_tag);
			//entry.var = m_string_list->getString(var_string_tag);

			table_data.entries.push_back(entry);
			current_pos = file.tellg();
		}

		table_section_data.tables.push_back(table_data);

	}

	return table_section_data;
}

void Parameters::printTableSectionData(TableSectionData table_section_data) {
	cout << "Table Section Data" << endl;
	int index = 0;
	for (int i = 0; i < table_section_data.tables.size(); i++) {
		cout << "Table " << i << endl;
		for (int j = 0; j < table_section_data.tables[i].entries.size(); j++) {
			cout << "Entry " << hex << index << ": " << table_section_data.tables[i].entries[j].name << " | ";
			index += 1;
			cout << table_section_data.tables[i].entries[j].var << endl;
		}
	}

}

Parameters::StructureSectionData Parameters::loadStructureSection(fstream& file)
{
	StructureSectionData structure_section;

	vector<int> section_addresses;

	map<int, int> address_to_section_number;



	// load addresses
	int last_vector_index = 0;
	for (int i = 0; i < 12; i++) {
		int entry_address;
		readIntFromStream(file, is_big_endian, 4, entry_address);
		if (entry_address == m_section_3_start) {
			break;
		}
		
		// handle repeating addresses
		if (i > 0) {
			// if this address is repeated
			if (entry_address == section_addresses[last_vector_index]) {
				address_to_section_number[entry_address] = i;
			} else {
				last_vector_index += 1;
				section_addresses.push_back(entry_address);
				address_to_section_number[entry_address] = i;
			}
		} else {
			last_vector_index = i;
			section_addresses.push_back(entry_address);
			address_to_section_number[entry_address] = i;
		}

	}

	int index = 0;




	for (int i = 0; i < section_addresses.size(); i++) {

		int section_address = section_addresses[i];

		// get entry lengths for section
		int section_number = address_to_section_number[section_address];
		int entry_length = structure_entry_lengths[section_number];

		file.seekg(section_address, ios::beg);

		int end_address;
		if (i + 1 < section_addresses.size()) {
			end_address = section_addresses[i + 1];
		}
		else {
			end_address = m_section_3_start;
		}

		vector<ParameterNode> nodes;


		while (file.tellg() < end_address) {
			ParameterNode node;
			node.loadStringList(m_string_list);
			node.load(file, entry_length, section_number);
			node.setIndex(index);
			index += 1;
			nodes.push_back(node);
			structure_section.parameter_nodes.push_back(node);
		}

	}

	return structure_section;
}

map<int, int> Parameters::structure_entry_lengths = {
	{0, 0x10},
	{1, 0x4},
	{2, 0x10},
	{3, 0x4},
	{4, 0x10},
	{5, 0x4},
	{6, 0x10},
	{7, 0x4},
	{8, 0x18},
	{9, 0x4},
	{10, 0x14},
	{11, 0x4}
};
