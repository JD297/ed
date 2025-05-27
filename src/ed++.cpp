#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <regex>
#include <algorithm>
#include <cctype>
#include <unistd.h>

typedef struct ed_state {
	std::list<std::string> buffer;
	std::list<std::string>::iterator addr_iter_current;
	std::list<std::string>::iterator addr_iter_begin;
	std::list<std::string>::iterator addr_iter_end;

	std::string filename;

	bool runs;

	bool printPrompt;
	std::string promt;

	bool printErrors;
	std::string error;

	std::string parameters;
} ed_state;

extern int run_command(ed_state *state, std::string command);

void ed_state_init(ed_state *state)
{
	state->promt = "*";
	state->printPrompt = false;
	state->filename = "src/ed++.cpp";
	state->buffer = std::list<std::string>();
	state->runs = true;
}

void ed_error(ed_state *state)
{
	std::cout << "?" << std::endl;

	if (state->printErrors) {
		std::cout << state->error << std::endl;
	}
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
	std::regex pattern("^/([^/]+)/([a-zA-Z])");
	std::smatch matches;

	std::regex_search(state->parameters, matches, pattern);

	if (matches.size() < 3) {
		// TODO set error msg
		return -1;
	}

	std::string global_command = matches[2];

	std::regex except_commands("[gGvV]");
	std::smatch except_match;

	if (std::regex_search(global_command,  except_match,  except_commands)) {
		// TODO set error msg
		return -1;
	}

	std::string search_pattern = matches[1];
	std::regex regex_search_pattern(search_pattern);
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
		state->addr_iter_begin = *it;
		state->addr_iter_end = *it;

		if (run_command(state, global_command) != 0) {
			ed_error(state);
		}
	}

	return 0;
}

int run_command(ed_state *state, std::string command)
{
	if (command.compare("q") == 0) {
		return command_quit(state);
	}
	else if (command.compare("e") == 0) {
		return command_edit(state);
	}
	else if (command.compare("f") == 0) {
		return command_file(state);
	}
	else if (command.compare("p") == 0) {
		return command_print(state);
	}
	else if (command.compare("n") == 0) {
		return command_number(state);
	}
	else if (command.compare("d") == 0) {
		return command_delete(state);
	}
	else if (command.compare("P") == 0) {
		return command_prompt(state);
	}
	else if (command.compare("w") == 0) {
		return command_write(state);
	}
	else if (command.compare("a") == 0) {
		return command_append(state);
	}
	else if (command.compare("i") == 0) {
		return command_insert(state);
	}
	else if (command.compare("s") == 0) {
		return command_substitute(state);
	}
	else if (command.compare("g") == 0) {
		return command_global(state);
	} else {
		// TODO set error msg
		return -1;
	}
}

int main()
{
	ed_state state;
	ed_state_init(&state);

	if (command_edit(&state) != 0) {
		ed_error(&state);
	}

	do {
		std::string cmd = "";

		if (state.printPrompt) {
			std::cout << state.promt << std::flush;
		}

		ssize_t nread;

		for (char buf[1]; ; ) {
			if ((nread = read(STDIN_FILENO, buf, 1)) == -1) {
				// err(EXIT_FAILURE, "read: cmd");
			}

			if (nread == 0) {
				break;
			}

			if (buf[0] == '\n') {
				break;
			}

			cmd.append(buf);
		}

		if (nread == 0) {
			break;
		}

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
				// TODO set error msg
				ed_error(&state);
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
				// TODO set error msg
				ed_error(&state);
			}
		}
		else if (address_start_match.length() > 0) {
			state.addr_iter_end = state.addr_iter_begin;
		}

		if (command_match.length() > 0) {
			std::string match = command_match[1];

			state.parameters = cmd_com.substr(match.length());

			if (run_command(&state, match) != 0) {
				ed_error(&state);
			}
		}


		state.addr_iter_current = state.addr_iter_end;
	} while (state.runs);

	return 0;
}
