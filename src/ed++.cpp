#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <regex>
#include <algorithm>
#include <cctype>

typedef struct ed_state {
	std::list<std::string> buffer;
	std::list<std::string>::iterator addr_iter_current;
	std::list<std::string>::iterator addr_iter_begin;
	std::list<std::string>::iterator addr_iter_end;

	std::string filename;

	bool runs;

	bool printPrompt;
	std::string promt;

	std::string parameters;
} ed_state;

void ed_state_init(ed_state *state)
{
	state->promt = "*";
	state->printPrompt = false;
	state->filename = "src/ed++.cpp";
	state->buffer = std::list<std::string>();
	state->runs = true;
}

int command_quit(ed_state *state)
{
	state->runs = false;

	return 0;
}

int command_file(ed_state *state)
{
	std::regex pattern("^\\s\\s*(.*)");
	std::smatch matches;

	std::regex_search(state->parameters, matches, pattern);

	if (matches.size() > 1) {
		state->filename = matches[1];
	}

	std::cout << state->filename << "\n";

	return 0;
}

int command_edit(ed_state *state)
{
	std::regex pattern("^\\s\\s*(.*)");
	std::smatch matches;

	std::regex_search(state->parameters, matches, pattern);

	if (matches.size() > 1) {
		state->filename = matches[1];
	}

	state->buffer.clear();

	std::ifstream file(state->filename);

	size_t nread = 0;

	for (std::string line; std::getline(file, line);) {
		state->buffer.push_back(line);

		nread += line.length() + 1;
	}

	std::cout << nread << "\n";

	state->addr_iter_current = std::prev(state->buffer.end());

	return 0;
}

int command_print(ed_state *state)
{
	for (auto it = state->addr_iter_begin; it != std::next(state->addr_iter_end); it++) {
		std::cout << *it << "\n";
	}

	return 0;
}

int command_number(ed_state *state)
{
	int line = std::distance(state->buffer.begin(), state->addr_iter_begin) + 1;

	for (auto it = state->addr_iter_begin; it != std::next(state->addr_iter_end); it++) {
		std::cout << line << "\t" << *it << "\n";

		line++;
	}

	return 0;
}

int command_delete(ed_state *state)
{
	auto addr_iter_temp = std::next(state->addr_iter_end);

	if (addr_iter_temp == std::prev(state->buffer.end()) || addr_iter_temp == state->buffer.end()) {
		addr_iter_temp = std::prev(state->addr_iter_begin);
	}

	auto end = std::next(state->addr_iter_end);

	for (auto it = state->addr_iter_begin; it != end; ) {
		it = state->buffer.erase(it);
	}

	state->addr_iter_end = addr_iter_temp;

	return 0;
}

int command_write(ed_state *state)
{
	std::ofstream outfile(state->filename);

	for (auto it = state->buffer.begin(); it != state->buffer.end(); it++) {
		outfile << *it << "\n";
	}

	std::cout << outfile.tellp() << "\n";

	return 0;
}

int command_prompt(ed_state *state)
{
	state->printPrompt = !state->printPrompt;

	return 0;
}

int command_append(ed_state *state)
{
	state->addr_iter_end = state->addr_iter_begin;

	while (1) {
		std::string line;

		std::getline(std::cin, line);

		if (line.compare(".") == 0)
		{
			break;
		}

		state->buffer.insert(std::next(state->addr_iter_end), line);

		state->addr_iter_end = std::next(state->addr_iter_end);
	}

	return 0;
}

int command_insert(ed_state *state)
{
	state->addr_iter_end = state->addr_iter_begin;

	while (1) {
		std::string line;

		std::getline(std::cin, line);

		if (line.compare(".") == 0)
		{
			break;
		}

		state->buffer.insert(state->addr_iter_begin, line);

		state->addr_iter_end = std::prev(state->addr_iter_begin);
	}

	return 0;
}

int command_substitute(ed_state *state)
{
	std::regex pattern("^/([^/]+)/([^/]+)/");
	std::smatch matches;

	std::regex_search(state->parameters, matches, pattern);

	if (matches.size() < 2) {
		return -1;
	}

	std::string rsp = matches[1];
	std::string rpl = matches[2];
	std::regex regex_search_pattern(rsp);

	for (auto it = state->addr_iter_begin; it != std::next(state->addr_iter_end); it++) {
		*it = std::regex_replace(*it, regex_search_pattern, rpl);
	}

	return 0;
}

int command_global(ed_state *state)
{
	std::regex pattern("^/([^/]+)/[np]");
	std::smatch matches;

	std::regex_search(state->parameters, matches, pattern);

	if (matches.size() < 2) {
		return -1;
	}

	std::string sp = matches[1];
	std::string gc = matches[2];
	std::regex regex_search_pattern(sp);
	std::smatch match;

	auto end = std::next(state->addr_iter_end);

	std::list<std::list<std::string>::iterator> marked_lines;

	for (auto it = state->addr_iter_begin; it != end; it++) {
		if (!std::regex_search(*it, match,regex_search_pattern)) {
			continue;
		}

		marked_lines.push_back(it);
	}

	for (auto it = marked_lines.begin(); it != marked_lines.end(); it++) {
		// TODO eval command: gc string to run the correct command and not
		// just print like function
		state->addr_iter_begin = *it;
		state->addr_iter_end = *it;

		if (command_print(state) != 0) {
			// TODO error
		}
	}

	return 0;
}

int main()
{
	ed_state state;
	ed_state_init(&state);

	if (command_edit(&state) != 0) {
		// TODO error printing
	}

	do {
		std::string cmd;

		if (state.printPrompt) {
			std::cout << state.promt;
		}

		std::getline(std::cin, cmd);

		std::regex address_pattern("^([0-9]+|[$]|[.])");

		std::smatch address_start_match;

		if (std::regex_search(cmd, address_start_match, address_pattern)) {
//			std::cout << "Match found (address_start_match): " << address_start_match.str() << std::endl;
//			std::cout << "Length: " << address_start_match.length() << std::endl;
		} else {
//			std::cout << "No match found (start)." << std::endl;
		}

		std::regex address_seperator_pattern("^([,]|[;])");
		std::smatch address_seperator_match;

		std::string cmd_sep = cmd.substr(address_start_match.length());

		if (std::regex_search(cmd_sep, address_seperator_match, address_seperator_pattern)) {
//			std::cout << "Match found (address_seperator_match): " << address_seperator_match.str() << std::endl;
//			std::cout << "Length: " << address_seperator_match.length() << std::endl;
		} else {
//			std::cout << "No match found (seperator)." << std::endl;
		}

		std::smatch address_end_match;

		std::string cmd_end = cmd_sep.substr(address_seperator_match.length());

		if (std::regex_search(cmd_end, address_end_match, address_pattern)) {
//			std::cout << "Match found (address_end_match): " << address_end_match.str() << std::endl;
//			std::cout << "Length: " << address_end_match.length() << std::endl;
		} else {
//			std::cout << "No match found (end)." << std::endl;
		}

		std::regex command_pattern("^([a-zA-Z])(.*+)");
		std::smatch command_match;

		std::string cmd_com = cmd_end.substr(address_end_match.length());

		if (std::regex_search(cmd_com, command_match, command_pattern)) {
//			std::cout << "Match found: " << command_match.str() << std::endl;
//			std::cout << "Length: " << command_match.length() << std::endl;
//			for (long int i = 0; i < command_match.length(); i++)
//				std::cout << "match[" << i << "]:" << command_match[i] << std::endl;

		} else {
//			std::cout << "No match found (command)." << std::endl;
		}


		if (address_start_match.length() == 0 && address_end_match.length() == 0) {
			if (address_seperator_match.length() == 0) {
				state.addr_iter_begin = state.addr_iter_current;
				state.addr_iter_end = state.addr_iter_current;
			}
			else {
				std::string match = address_seperator_match.str();

				if (match.compare(",") == 0) {
					state.addr_iter_begin = state.buffer.begin();
					state.addr_iter_end = std::prev(state.buffer.end());
				}
				else if (match.compare(";") == 0) {
					state.addr_iter_begin = state.addr_iter_current;
					state.addr_iter_end = std::prev(state.buffer.end());
				}
			}
		}

		if (address_start_match.length() > 0) {
			std::string match = address_start_match.str();

			if (match.compare("$") == 0) {
				state.addr_iter_begin = std::prev(state.buffer.end());
			}
			else if (match.compare(".") == 0) {
				state.addr_iter_begin = state.addr_iter_current;
			}
			else if (std::all_of(match.begin(), match.end(), ::isdigit)) {
				state.addr_iter_begin = std::next(state.buffer.begin(), std::stoi(match) - 1);
			}
			else {
				// std::cout << "Error interpreting (start): " << match << std::endl;
				// TODO error
			}
		}

		if (address_end_match.length() > 0) {
			std::string match = address_end_match.str();

			if (match.compare("$") == 0) {
				state.addr_iter_end = std::prev(state.buffer.end());
			}
			else if (match.compare(".") == 0) {
				state.addr_iter_end = state.addr_iter_current;
			}
			else if (std::all_of(match.begin(), match.end(), ::isdigit)) {
				state.addr_iter_end = std::next(state.buffer.begin(), std::stoi(match) - 1);
			}
			else {
				//std::cout << "Error interpreting (end)" << std::endl;
				// TODO error
			}
		}
		else if (address_start_match.length() > 0) {
			state.addr_iter_end = state.addr_iter_begin;
		}

		if (command_match.length() > 0) {
			std::string match = command_match[1];

			state.parameters = cmd_com.substr(match.length());

			if (match.compare("q") == 0) {
				if (command_quit(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("e") == 0) {
				if (command_edit(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("f") == 0) {
				if (command_file(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("p") == 0) {
				if (command_print(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("n") == 0) {
				if (command_number(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("d") == 0) {
				if (command_delete(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("P") == 0) {
				if (command_prompt(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("w") == 0) {
				if (command_write(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("a") == 0) {
				if (command_append(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("i") == 0) {
				if (command_insert(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("s") == 0) {
				if (command_substitute(&state) != 0) {
					// TODO error
				}
			}
			else if (match.compare("g") == 0) {
				if (command_global(&state) != 0) {
					// TODO error
				}
			} else {
				// TODO error
			}
		}


		state.addr_iter_current = state.addr_iter_end;
	} while (state.runs);

	return 0;
}
