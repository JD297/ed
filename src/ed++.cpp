#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <regex>
#include <algorithm>
#include <cctype>
#include <unistd.h>

typedef enum {
	UNCHANGED,
	CHANGED,
	CHANGED_AND_WARNED
} ModState;

typedef struct ed_state {
	std::list<std::string> buffer;
	std::list<std::string>::iterator addr_iter_current;
	std::list<std::string>::iterator addr_iter_begin;
	std::list<std::string>::iterator addr_iter_end;

	std::string filename;

	bool runs;

	bool prompt_print;
	std::string promt;

	bool script;

	bool error_print;
	std::string error;

	std::string cmd;

	ModState mod_state;

	bool has_cmd_addr;
} ed_state;

extern int run_command(ed_state *state, std::string command);

void ed_state_init(ed_state *state)
{
	state->promt = "*";
	state->prompt_print = false;

	state->error = "";
	state->error_print = false;

	state->script = false;

	state->filename = "";

	state->cmd = "";

	state->buffer = std::list<std::string>();
	state->addr_iter_current = state->buffer.begin();

	state->addr_iter_begin = state->addr_iter_end = state->buffer.begin();

	state->runs = true;

	state->mod_state = UNCHANGED;

	state->has_cmd_addr = false;
}

int command_print_error(ed_state *state)
{
	if (state->error.length() > 0) {
		std::cout << state->error << std::endl;
	}

	return 0;
}

void ed_error(ed_state *state)
{
	std::cout << "?" << std::endl;

	if (state->error_print) {
		command_print_error(state);
	}
}

int command_toggle_print_error(ed_state *state)
{
	if ((state->error_print = !state->error_print)) {
		return command_print_error(state);
	}

	return 0;
}

int command_quit(ed_state *state)
{
	if (state->mod_state == CHANGED) {
		state->mod_state = CHANGED_AND_WARNED;

		state->error = "Warning: buffer modified";

		return -1;
	}

	state->runs = false;

	return 0;
}

int command_file(ed_state *state)
{
	std::regex pattern("^\\s\\s*(.*)");
	std::smatch matches;

	std::regex_search(state->cmd, matches, pattern);

	if (matches.size() > 1) {
		state->filename = matches[1];
	}

	if (state->filename.empty()) {
		state->error = "No current filename";

		return -1;
	}

	std::cout << state->filename << std::endl;

	return 0;
}

int command_edit(ed_state *state)
{
	if (state->mod_state == CHANGED) {
		state->mod_state = CHANGED_AND_WARNED;

		state->error = "Warning: buffer modified";

		return -1;
	}

	std::regex pattern("^\\s\\s*(.*)");
	std::smatch matches;

	std::regex_search(state->cmd, matches, pattern);

	if (matches.size() > 1) {
		state->filename = matches[1];
	}

	if (state->filename.empty()) {
		state->error = "No current filename";

		return -1;
	}

	state->buffer.clear();

	std::ifstream file(state->filename);

	size_t nread = 0;

	for (std::string line; std::getline(file, line);) {
		state->buffer.push_back(line);

		nread += line.length() + 1;
	}

	if (!state->script) {
		std::cout << nread << std::endl;
	}

	state->addr_iter_current = std::prev(state->buffer.end());

	state->mod_state = UNCHANGED;

	return 0;
}

int command_print(ed_state *state)
{
	for (auto it = state->addr_iter_begin; it != std::next(state->addr_iter_end); it++) {
		std::cout << *it << std::endl;
	}

	state->addr_iter_current = state->addr_iter_end;

	return 0;
}

int command_number(ed_state *state)
{
	int line = std::distance(state->buffer.begin(), state->addr_iter_begin) + 1;

	for (auto it = state->addr_iter_begin; it != std::next(state->addr_iter_end); it++) {
		std::cout << line << "\t" << *it << std::endl;

		line++;
	}

	state->addr_iter_current = state->addr_iter_end;

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

	state->addr_iter_current = addr_iter_temp;

	state->mod_state = CHANGED;

	return 0;
}

int command_write(ed_state *state)
{
	if (state->filename.empty()) {
		state->error = "No current filename";

		return -1;
	}

	std::ofstream outfile(state->filename);

	if (!state->has_cmd_addr) {
		state->addr_iter_begin = state->buffer.begin();
		state->addr_iter_end = std::prev(state->buffer.end());
	}

	for (auto it = state->addr_iter_begin; it != std::next(state->addr_iter_end); it++) {
		outfile << *it << std::endl;
	}

	if (!state->script) {
		std::cout << outfile.tellp() << std::endl;
	}

	state->mod_state = UNCHANGED;

	return 0;
}

int command_prompt(ed_state *state)
{
	state->prompt_print = !state->prompt_print;

	return 0;
}

int command_append(ed_state *state)
{
	state->addr_iter_current = state->addr_iter_begin;

	while (1) {
		std::string line;

		std::getline(std::cin, line);

		if (line.compare(".") == 0)
		{
			break;
		}

		state->buffer.insert(std::next(state->addr_iter_current), line);

		state->addr_iter_current = std::next(state->addr_iter_current);
	}

	state->mod_state = CHANGED;

	return 0;
}

int command_insert(ed_state *state)
{
	state->addr_iter_current = state->addr_iter_begin;

	while (1) {
		std::string line;

		std::getline(std::cin, line);

		if (line.compare(".") == 0)
		{
			break;
		}

		state->buffer.insert(state->addr_iter_begin, line);

		state->addr_iter_current = std::prev(state->addr_iter_begin);
	}

	state->mod_state = CHANGED;

	return 0;
}

int command_substitute(ed_state *state)
{
	std::regex pattern("^/([^/]+)/([^/]+)/");
	std::smatch matches;

	std::regex_search(state->cmd, matches, pattern);

	if (matches.size() < 2) {
		return -1;
	}

	std::string rsp = matches[1];
	std::string rpl = matches[2];
	std::regex regex_search_pattern(rsp);

	for (auto it = state->addr_iter_begin; it != std::next(state->addr_iter_end); it++) {
		*it = std::regex_replace(*it, regex_search_pattern, rpl);
	}

	state->addr_iter_current = state->addr_iter_end;

	state->mod_state = CHANGED;

	return 0;
}

int command_global(ed_state *state)
{
	std::regex pattern("^/([^/]+)/?([a-zA-Z])?");
	std::smatch matches;

	std::regex_search(state->cmd, matches, pattern);

	std::string global_command = "p";

	//for (size_t i = 0; i < matches.size(); i++)
	//	std::cout << "match[" << i << "]: \"" << matches[i] << "\"" << std::endl;

	// TODO if RE  is empty then use last RE
	// if no pattern then error "No previous pattern"

	if (matches.size() == 0) {
		state->error = "Invalid pattern delimiter";
		return -1;
	} else if (matches.size() == 3 && matches[2] != "") {
		global_command = matches[2];
	}

	std::regex except_commands("[gGvV]");
	std::smatch except_match;

	if (std::regex_search(global_command, except_match, except_commands)) {
		state->error = "Cannot nest global commands";
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
			// ed_error(state); // TODO remove
			break;
		}
	}

	state->addr_iter_current = state->addr_iter_end;

	return 0;
}

int command_line_number(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_iter_end = std::prev(state->buffer.end());
	}

	std::cout << std::distance(state->buffer.begin(), state->addr_iter_end) + 1 << std::endl;

	return 0;
}

int run_command(ed_state *state, std::string command)
{
	if (command.compare("q") == 0) {
		return command_quit(state);
	}
	else if (command.compare("=") == 0) {
		return command_line_number(state);
	}
	else if (command.compare("Q") == 0) {
		state->mod_state = CHANGED_AND_WARNED;

		return command_quit(state);
	}
	else if (command.compare("e") == 0) {
		return command_edit(state);
	}
	else if (command.compare("E") == 0) {
		state->mod_state = CHANGED_AND_WARNED;

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
	else if (command.compare("h") == 0) {
		return command_print_error(state);
	}
	else if (command.compare("H") == 0) {
		return command_toggle_print_error(state);
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
	}
	else if (state->cmd.empty()) {
		return command_print(state);
	}
	else {
		state->error = "Unknown command";
		return -1;
	}
}

void print_usage()
{
	std::cerr << "Usage: " << TARGET << " [-p string] [-s] [file]" << std::endl;
	std::cerr << TARGET << " — edit text" << std::endl;
	std::cerr << std::endl;
	std::cerr << "JD297 " << TARGET << " source code <https://github.com/jd297/ed>" << std::endl;
}

int interpret_addr_part(ed_state *state, std::list<std::string>::iterator *addr_it, ssize_t line_offset, ssize_t line_max)
{
	std::regex r_pattern("^([\\s]+|[+-]([0-9]+)*|[0-9]+|[$]|[.])");
	std::smatch r_match;

	bool first_run;

	for(first_run = true; std::regex_search(state->cmd, r_match, r_pattern); first_run = false) {
		state->has_cmd_addr = true;

		#ifdef DEBUG_ADDR
		std::cout << "===interpret_addr_part DBG:" << std::endl;
		std::cout << "\tOffset" << line_offset << std::endl;
		std::cout << "\tMax" << line_max << std::endl;
		std::cout << "\tMatch found: " << r_match.str() << std::endl;
		std::cout << "\tLength: " << r_match.length() << std::endl;

		/*for (ssize_t i = 0; i < r_match.length(); i++) {
			std::cout << "\tmatch[" << i << "]:" << r_match[i] << std::endl;
		}*/
		#endif

		std::string match = r_match.str();
		state->cmd = state->cmd.substr(match.length());

		#ifdef DEBUG_ADDR
		std::cout << "\tCMD (SUBSTR): \"" << state->cmd << "\"" << std::endl;
		#endif

		if (match.compare("$") == 0) {
			if (!first_run) {
				state->error = "Invalid address";
				ed_error(state);
				return -1;
			}

			line_offset = line_max;
		}
		else if (match.compare(".") == 0) {
			if (!first_run) {
				state->error = "Invalid address";
				ed_error(state);
				return -1;
			}
		}
		else if (std::all_of(match.begin(), match.end(), ::isdigit)) {
			long number = std::stol(match);

			if (first_run) {
				line_offset = number;
			} else {
				line_offset += number;
			}
		}
		else if (match.compare("+") == 0) {
			line_offset++;
		}
		else if (match.compare("-") == 0) {
			line_offset--;
		}
		else if (match.length() >= 2) {
			long number = std::stol(match.substr(1));

			std::string token = match.substr(0, 1);

			if (token.compare("+") == 0) {
				line_offset += number;
			}
			else if (token.compare("-") == 0) {
				line_offset -= number;
			}
		}
	}

	if (first_run && r_match.length() == 0) {
		return 1;
	}

	if (line_offset > line_max || line_offset < 1) {
		state->error = "Invalid address";
		ed_error(state);
		return -1;
	}

	*addr_it = std::next(state->buffer.begin(), line_offset - 1);

	return 0;
}

int main(int argc, char **argv)
{
	ed_state state;
	ed_state_init(&state);

	int opt;

	while ((opt = getopt(argc, argv, "sp:")) != -1) {
		switch (opt) {
			case 'p':
				state.promt = optarg;
				state.prompt_print = true;
				break;
			case 's':
				state.script = 1;
				break;
			default:
				print_usage();
				exit(EXIT_FAILURE);
		}
	}

	if (optind < argc) {
		state.filename = argv[optind];

		if (command_edit(&state) != 0) {
			ed_error(&state);
		}
	}

	do {
		state.has_cmd_addr = false;

		state.cmd = "";

		if (state.prompt_print) {
			std::cout << state.promt << std::flush;
		}

		while (int ch = getchar()) {
			if (state.cmd.length() == 0 && ch == '\n') {
				state.cmd = "+1p";
				break;
			}

			if (ch == EOF || ch == '\n') {
				break;
			}

			state.cmd += ch;
		}

		if (state.cmd.length() == 0) {
			state.cmd = "q";
		}

		ssize_t line_offset = std::distance(state.buffer.begin(), state.addr_iter_current) + 1;
		ssize_t line_max = std::distance(state.addr_iter_current, state.buffer.end()) - 1 + line_offset;

		int interpret_result_begin = interpret_addr_part(&state, &state.addr_iter_begin, line_offset, line_max);

		if (interpret_result_begin == -1) {
			continue;
		}

		std::regex address_seperator_pattern("^([,]|[;])");
		std::smatch address_seperator_match;
		std::string address_seperator_match_str = "";

		#ifdef DEBUG_ADDR
		std::cout << "===SEPERATOR DBG:" << std::endl;
		#endif

		if (std::regex_search(state.cmd, address_seperator_match, address_seperator_pattern)) {
			address_seperator_match_str = address_seperator_match.str();

			state.cmd = state.cmd.substr(address_seperator_match_str.length());
			state.has_cmd_addr = true;

			#ifdef DEBUG_ADDR
			std::cout << "\tMatch found (address_seperator_match): " << address_seperator_match_str << std::endl;
			#endif

		} else {
			#ifdef DEBUG_ADDR
			std::cout << "\tNo match found (seperator)." << std::endl;
			#endif
		}

		int interpret_result_end = interpret_addr_part(&state, &state.addr_iter_end, line_offset, line_max);

		if (interpret_result_end == -1) {
			continue;
		}

		std::regex command_pattern("^([a-zA-Z=])(.*+)");
		std::smatch command_match;

		#ifdef DEBUG_CMD
		std::cout << "===COMMAND DBG:" << std::endl;
		#endif

		if (std::regex_search(state.cmd, command_match, command_pattern)) {
			#ifdef DEBUG_CMD
			std::cout << "\tMatch found: " << command_match.str() << std::endl;
			std::cout << "\tLength: " << command_match.length() << std::endl;
			for (long int i = 0; i < command_match.length(); i++)
				std::cout << "\tmatch[" << i << "]:" << command_match[i] << std::endl;
			#endif

		} else {
			#ifdef DEBUG_CMD
			std::cout << "\tNo match found (command)." << std::endl;
			#endif
		}

		if (interpret_result_begin == 1 && interpret_result_end == 1) {
			if (address_seperator_match_str.empty()) {
				state.addr_iter_begin = state.addr_iter_current;
				state.addr_iter_end = state.addr_iter_current;
			}
			else if (address_seperator_match_str.compare(",") == 0) {
				state.addr_iter_begin = state.buffer.begin();
				state.addr_iter_end = std::prev(state.buffer.end());
			}
			else if (address_seperator_match_str.compare(";") == 0) {
				state.addr_iter_begin = state.addr_iter_current;
				state.addr_iter_end = std::prev(state.buffer.end());
			}
		}
		else if (interpret_result_end == 1 && interpret_result_begin == 0) {
			state.addr_iter_end = state.addr_iter_begin;
		}

		if (
			(state.addr_iter_begin == state.buffer.end() && !state.buffer.empty())
				||
			(state.addr_iter_end == state.buffer.end() && !state.buffer.empty())
				||
			(
				std::distance(state.buffer.begin(), state.addr_iter_end)
					<
				std::distance(state.buffer.begin(), state.addr_iter_begin)
			)

		) {
			state.error = "Invalid address";
			ed_error(&state);
			continue;
		}

		std::string match = "";

		if (command_match.length() > 0) {
			match = command_match[1];
		}

		state.cmd = state.cmd.substr(match.length());

		if (run_command(&state, match) != 0) {
			ed_error(&state);
		}
	} while (state.runs);

	return 0;
}
