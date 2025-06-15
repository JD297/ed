#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <regex>
#include <algorithm>
#include <cctype>
#include <unistd.h>
#include <iomanip>
#include <sys/ioctl.h>

typedef enum {
	UNCHANGED,
	CHANGED,
	CHANGED_AND_WARNED
} ModState;

typedef struct ed_state {
	std::list<std::string> buffer;
	std::list<std::string>::iterator addr_iter_cur;
	std::list<std::string>::iterator addr_iter_begin;
	std::list<std::string>::iterator addr_iter_end;
	ssize_t addr_offset_cur;
	ssize_t addr_offset_begin;
	ssize_t addr_offset_end;
	ssize_t addr_offset_max;
	std::list<std::string>::iterator addr_iter_marked['z'];

	std::string filename;

	bool runs;

	bool prompt_print;
	std::string promt;

	bool script;

	bool error_print;
	std::string error;

	std::string input;

	char cmd;

	std::smatch params;

	char suffix;

	ModState mod_state;

	bool has_cmd_addr;

	int num_addr;

	std::string last_re;
} ed_state;

#define REGEX_ADDR "^([\\s]+|[+-]([0-9]+)*|[0-9]+|[$]|[.]|[?]|[/]|'.?|[,;])*"
#define REGEX_ADDR_PART "^([\\s]+|[+-]([0-9]+)*|[0-9]+|[$]|[.]|[?]|[/]|'.?)"
#define REGEX_ADDR_SEPERATOR "^[,;]"
#define REGEX_CMD "^\\s*([acdEefGghHijklmnpPQqrstuVvw=!]|$)"
#define REGEX_PARAM_SUBSTITUTE "^/([^/]*)/([^/]*)/?"
#define REGEX_PARAM_GLOBAL "^/([^/]*)/?([a-zA-Z])?"
#define REGEX_PARAM_FILE "^\\s\\s*(.*)"
#define REGEX_PARAM_MARK "^."
#define REGEX_GLOBAL_CMD_EXCEPT "[gGvV]"

extern int run_command(ed_state *state);

extern int interpret_addr(ed_state *state);

extern int validate_addr(ed_state *state,
                         int expected_num_addr, bool allow_zero_addr);

#define VALIDATE_ADDR_EXPECT_NO_ADDR()\
	if (validate_addr(state, 0, false) != 0) {\
		return -1;\
	}

#define VALIDATE_ADDR_EXPECT_MULTI_ADDR()\
	if (validate_addr(state, 2, false) != 0) {\
		return -1;\
	}

#define VALIDATE_ADDR_EXPECT_SINGLE_ADDR_NON_ZERO()\
	if (validate_addr(state, 1, false) != 0) {\
		return -1;\
	}

#define VALIDATE_ADDR_EXPECT_SINGLE_ADDR_ZERO()\
	if (validate_addr(state, 1, true) != 0) {\
		return -1;\
	}

void invalid_marked_addr_iter(ed_state *state,
                              std::list<std::string>::iterator it)
{
	for (int i = 0; i < 'z'; i++) {
		if (state->addr_iter_marked[i] == it) {
			state->addr_iter_marked[i] = state->buffer.end();
		}
	}
}

void ed_state_init(ed_state *state)
{
	state->promt = "*";
	state->prompt_print = false;

	state->error = "";
	state->error_print = false;

	state->script = false;

	state->filename = "";

	state->input = "";

	state->buffer = std::list<std::string>();

	state->addr_offset_cur = 0;
	state->addr_offset_begin = 0;
	state->addr_offset_end = 0;
	state->addr_offset_max = 0;

	std::fill(std::begin(state->addr_iter_marked),
	          std::end(state->addr_iter_marked),
	          state->buffer.end());

	state->runs = true;

	state->mod_state = UNCHANGED;

	state->has_cmd_addr = false;

	state->num_addr = 0;

	state->last_re = "";
}

void ed_state_set_addr_iter(ed_state *state)
{
	if (state->addr_offset_begin == 0) {
		state->addr_iter_begin = state->buffer.begin();
	} else {
		state->addr_iter_begin = std::next(state->buffer.begin(),
		                                   state->addr_offset_begin - 1);
	}

	if (state->addr_offset_end == 0) {
		state->addr_iter_end = state->buffer.begin();
	} else {
		state->addr_iter_end = std::next(state->buffer.begin(),
		                                 state->addr_offset_end - 1);
	}
}

void ed_print_error(ed_state *state)
{
	if (state->error.length() > 0) {
		std::cout << state->error << std::endl;
	}
}

int command_help(ed_state *state)
{
	VALIDATE_ADDR_EXPECT_NO_ADDR();

	ed_print_error(state);

	return 0;
}

void ed_error(ed_state *state)
{
	std::cout << "?" << std::endl;

	if (state->error_print) {
		ed_print_error(state);
	}
}

int command_help_mode(ed_state *state)
{
	VALIDATE_ADDR_EXPECT_NO_ADDR();

	if ((state->error_print = !state->error_print)) {
		return command_help(state);
	}

	return 0;
}

int command_quit(ed_state *state)
{
	VALIDATE_ADDR_EXPECT_NO_ADDR();

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
	VALIDATE_ADDR_EXPECT_NO_ADDR();

	if (state->params.size() > 1) {
		state->filename = state->params[1];
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
	VALIDATE_ADDR_EXPECT_NO_ADDR();

	if (state->mod_state == CHANGED) {
		state->mod_state = CHANGED_AND_WARNED;

		state->error = "Warning: buffer modified";

		return -1;
	}

	if (state->params.size() > 1) {
		state->filename = state->params[1];
	}

	if (state->filename.empty()) {
		state->error = "No current filename";

		return -1;
	}

	state->buffer.clear();

	std::fill(std::begin(state->addr_iter_marked),
	          std::end(state->addr_iter_marked),
	          state->buffer.end());

	std::ifstream file(state->filename);

	size_t nread = 0;

	for (std::string line; std::getline(file, line);) {
		state->buffer.push_back(line);

		nread += line.length() + 1;
	}

	if (!state->script) {
		std::cout << nread << std::endl;
	}

	state->addr_offset_cur = state->buffer.size();

	state->mod_state = UNCHANGED;

	return 0;
}

int command_print(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	auto end = std::next(state->addr_iter_end);

	for (auto it = state->addr_iter_begin; it != end; it++) {
		std::cout << *it << std::endl;
	}

	state->addr_offset_cur = state->addr_offset_end;

	return 0;
}

int command_number(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	ssize_t line = state->addr_offset_begin;

	auto end = std::next(state->addr_iter_end);

	for (auto it = state->addr_iter_begin; it != end; it++) {
		std::cout << line << "\t" << *it << std::endl;

		line++;
	}

	state->addr_offset_cur = state->addr_offset_end;

	return 0;
}

int command_delete(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	auto addr_iter_temp = std::next(state->addr_iter_end);

	if (addr_iter_temp == state->buffer.end()) {
		addr_iter_temp = std::prev(state->addr_iter_begin);
	}

	auto end = std::next(state->addr_iter_end);

	for (auto it = state->addr_iter_begin; it != end; ) {
		invalid_marked_addr_iter(state, it);

		it = state->buffer.erase(it);
	}

	if (state->buffer.size() == 0) {
		state->addr_offset_cur = 0;
	} else {
		state->addr_offset_cur = 1 + std::distance(state->buffer.begin(),
		                                           addr_iter_temp);
	}

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
		state->addr_offset_begin = 1;
		state->addr_offset_end = state->addr_offset_max;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	auto end = std::next(state->addr_iter_end);

	for (auto it = state->addr_iter_begin; it != end; it++) {
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
	VALIDATE_ADDR_EXPECT_NO_ADDR();

	state->prompt_print = !state->prompt_print;

	return 0;
}

int command_append(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_SINGLE_ADDR_ZERO();

	ed_state_set_addr_iter(state);

	state->addr_iter_cur = state->addr_iter_begin;

	while (1) {
		std::string line;

		std::getline(std::cin, line);

		if (line.compare(".") == 0) {
			break;
		}

		if (state->addr_iter_cur == state->addr_iter_begin
			&&
			state->addr_offset_begin == 0) {
			state->buffer.insert(state->addr_iter_cur, line);

			state->addr_iter_cur = state->buffer.begin();
		} else {
			state->buffer.insert(std::next(state->addr_iter_cur), line);

			state->addr_iter_cur = std::next(state->addr_iter_cur);
		}

		state->mod_state = CHANGED;
	}

	state->addr_offset_cur = 1 + std::distance(state->buffer.begin(),
	                                           state->addr_iter_cur);

	return 0;
}

int command_insert(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_SINGLE_ADDR_ZERO();

	ed_state_set_addr_iter(state);

	state->addr_iter_cur = state->addr_iter_begin;

	while (1) {
		std::string line;

		std::getline(std::cin, line);

		if (line.compare(".") == 0)
		{
			break;
		}

		state->buffer.insert(state->addr_iter_begin, line);

		state->addr_iter_cur = std::prev(state->addr_iter_begin);

		state->mod_state = CHANGED;
	}

	if (state->addr_offset_cur > 0) {
		state->addr_offset_cur = 1 + std::distance(state->buffer.begin(),
		                                           state->addr_iter_cur);
	}

	return 0;
}

int command_change(ed_state *state)
{
	if (command_delete(state) != 0) {
		return -1;
	}

	state->has_cmd_addr = false;

	return command_insert(state);
}

int command_substitute(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	if (state->params.size() < 2) {
		return -1;
	}

	std::string search_pattern = state->last_re;

	if (state->params[1].length() > 0) {
		search_pattern = state->params[1];
		state->last_re = search_pattern;
	}

	if (search_pattern.length() == 0) {
		state->error = "No previous pattern";
		return -1;
	}

	std::regex regex_search_pattern(search_pattern);

	std::string rpl = state->params[2];

	auto end = std::next(state->addr_iter_end);

	for (auto it = state->addr_iter_begin; it != end; it++) {
		invalid_marked_addr_iter(state, it); // TODO only when a match happend

		*it = std::regex_replace(*it, regex_search_pattern, rpl);
	}

	state->addr_offset_cur = state->addr_offset_end;

	state->mod_state = CHANGED;

	return 0;
}

int command_global(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = 1;
		state->addr_offset_end = state->addr_offset_max;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	std::string global_command = "p";

	if (state->params.size() == 0) {
		state->error = "Invalid pattern delimiter";
		return -1;
	} else if (state->params.size() == 3 && state->params[2] != "") {
		global_command = state->params[2];
	}

	state->cmd = global_command.at(0);

	std::smatch except_match;

	if (std::regex_search(global_command, except_match,
		                  std::regex(REGEX_GLOBAL_CMD_EXCEPT))) {
		state->error = "Cannot nest global commands";
		return -1;
	}

	std::string search_pattern = state->last_re;

	if (state->params[1].length() > 0) {
		search_pattern = state->params[1];
		state->last_re = search_pattern;
	}

	if (search_pattern.length() == 0) {
		state->error = "No previous pattern";
		return -1;
	}

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
		state->has_cmd_addr = false;
		state->addr_offset_cur = 1 + std::distance(state->buffer.begin(),
		                                           *it);

		if (run_command(state) != 0) {
			return -1;
		}
	}

	return 0;
}

int command_line_number(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_end = state->addr_offset_max;
	}

	VALIDATE_ADDR_EXPECT_SINGLE_ADDR_ZERO();

	std::cout << state->addr_offset_end << std::endl;

	return 0;
}

int command_mark(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_SINGLE_ADDR_NON_ZERO();

	ed_state_set_addr_iter(state);

	if (state->params.size() == 0) {
		state->error = "Expected mark character";
		return -1;
	}

	int x = (int)state->params.str().at(0);

	if (islower(x) == 0) {
		state->error = "Invalid mark character";
		return -1;
	}

	state->addr_iter_marked[x - 'a'] = state->addr_iter_begin;

	return 0;
}

int command_list(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	size_t cols = 72;
	size_t printed = 0;

	#ifdef TIOCGWINSZ
	struct winsize w;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
		cols = w.ws_col - 8;
	}
	#endif

	auto end = std::next(state->addr_iter_end);

	for (auto it = state->addr_iter_begin; it != end; it++) {
		for (unsigned char ch : *it) {
			if (printed >= cols) {
				printed = 0;

				std::cout << "\\" << std::endl;
			}

			switch (ch) {
				case '\\': std::cout << "\\\\"; printed += 2; break;
				case '\a': std::cout << "\\a";  printed += 2; break;
				case '\b': std::cout << "\\b";  printed += 2; break;
				case '\f': std::cout << "\\f";  printed += 2; break;
				case '\r': std::cout << "\\r";  printed += 2; break;
				case '\t': std::cout << "\\t";  printed += 2; break;
				case '\v': std::cout << "\\v";  printed += 2; break;
				case '$' : std::cout << "\\$";  printed += 2; break;
				default: {
					if (isprint((int)ch) != 0) {
						std::cout << ch;

						++printed;
					} else {
						std::cout << "\\" << std::setfill('0')
						          << std::setw(3) << std::oct << (int)ch;

						printed += 4;
					}
				} break;
			}
		}

		std::cout << "$" << std::endl;

		printed = 0;
	}

	state->addr_offset_cur = state->addr_offset_end;

	return 0;
}

int command_move(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	ed_state move_state = *state;
	move_state.input = state->params.str();

	if (interpret_addr(&move_state) != 0) {
		return -1;
	}

	if (!move_state.has_cmd_addr) {
		move_state.addr_offset_begin = move_state.addr_offset_cur;
		move_state.addr_offset_end = move_state.addr_offset_cur;
	}

	if (validate_addr(&move_state, 1, true) != 0) {
		return -1;
	}

	if (
		move_state.addr_offset_end >= state->addr_offset_begin
			&&
		move_state.addr_offset_end <= state->addr_offset_end
	) {
		state->error = "Invalid destination";
		return -1;
	}

	auto addr_dst_iter = std::next(state->buffer.begin(),
	                               move_state.addr_offset_end);

	state->buffer.splice(addr_dst_iter,
						 state->buffer,
						 state->addr_iter_begin,
						 std::next(state->addr_iter_end));

	state->mod_state = CHANGED;

	ssize_t moved_lines = 1 + move_state.addr_offset_end
	                      - move_state.addr_offset_begin;

	ssize_t move_offset = move_state.addr_offset_end;

	if (move_offset > 0) {
		--move_offset;
	}

	state->addr_offset_cur = move_offset + moved_lines;

	return 0;
}

int command_copy(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	ed_state copy_state = *state;
	copy_state.input = state->params.str();

	if (interpret_addr(&copy_state) != 0) {
		return -1;
	}

	if (!copy_state.has_cmd_addr) {
		copy_state.addr_offset_begin = copy_state.addr_offset_cur;
		copy_state.addr_offset_end = copy_state.addr_offset_cur;
	}

	if (validate_addr(&copy_state, 1, true) != 0) {
		return -1;
	}

	if (
		copy_state.addr_offset_end >= state->addr_offset_begin
			&&
		copy_state.addr_offset_end <= state->addr_offset_end
	) {
		state->error = "Invalid destination";
		return -1;
	}

	auto addr_dst_iter = std::next(state->buffer.begin(),
	                               copy_state.addr_offset_end);

	state->buffer.insert(addr_dst_iter, state->addr_iter_begin,
	                     std::next(state->addr_iter_end));

	state->mod_state = CHANGED;

	ssize_t copied_lines = 1 + copy_state.addr_offset_end
	                       - copy_state.addr_offset_begin;

	ssize_t copy_offset = copy_state.addr_offset_end;

	if (copy_offset > 0) {
		--copy_offset;
	}

	state->addr_offset_cur = copy_offset + copied_lines;

	return 0;
}

int command_join(ed_state *state)
{
	if (!state->has_cmd_addr) {
		state->addr_offset_begin = state->addr_offset_cur;
		state->addr_offset_end = state->addr_offset_cur + 1;
	}

	VALIDATE_ADDR_EXPECT_MULTI_ADDR();

	ed_state_set_addr_iter(state);

	if (state->addr_offset_begin == state->addr_offset_end) {
		return 0;
	}

	auto end = std::next(state->addr_iter_end);

	for (auto it = std::next(state->addr_iter_begin); it != end; ) {
		state->addr_iter_begin->append(*it);
		invalid_marked_addr_iter(state, it);

		it = state->buffer.erase(it);
	}

	state->mod_state = CHANGED;

	state->addr_offset_cur = state->addr_offset_begin;

	return 0;
}

int command_null(ed_state *state)
{
	VALIDATE_ADDR_EXPECT_SINGLE_ADDR_NON_ZERO();

	return command_print(state);
}

int run_command(ed_state *state)
{
	switch (state->cmd) {
		case 'a': return command_append(state);

		case 'c': return command_change(state);

		case 'd': return command_delete(state);

		case 'E': state->mod_state = CHANGED_AND_WARNED; [[fallthrough]];
		case 'e': return command_edit(state);

		case 'f': return command_file(state);

		// TODO case 'G': state->interactive = true; [[fallthrough]];
		case 'g': return command_global(state);

		case 'h': return command_help(state);

		case 'H': return command_help_mode(state);

		case 'i': return command_insert(state);

		case 'j': return command_join(state);

		case 'k': return command_mark(state);

		case 'l': return command_list(state);

		case 'm': return command_move(state);

		case 'n': return command_number(state);

		case 'p': return command_print(state);

		case 'P': return command_prompt(state);

		case 'Q': state->mod_state = CHANGED_AND_WARNED; [[fallthrough]];
		case 'q': return command_quit(state);

		// TODO case 'r': return command_read(state);

		case 's': return command_substitute(state);

		case 't': return command_copy(state);

		// TODO case 'u': return command_undo(state);

		// TODO case 'V': state->interactive = true; [[fallthrough]];
		// TODO case 'v': state->non_matched = true; return command_global(state);

		case 'w': return command_write(state);

		case '=': return command_line_number(state);

		// TODO case '!': return command_shell_escape(state);

		case '\0': return command_null(state);

		default: state->error = "Command not implemented (" + std::string(1, state->cmd) + ")"; return -1;
	}
}

void print_usage(char **argv)
{
	std::cerr << "Usage: " << argv[0] << " [-p string] [-s] [file]"
	          << std::endl;
}

#define ADDR_PART_OK(offset) (offset) >= 0
#define ADDR_PART_ERROR -1
#define ADDR_PART_NO_MATCH -2

ssize_t interpret_addr_part(ed_state *state)
{
	ssize_t addr_offset = state->addr_offset_cur;

	std::smatch r_match;

	bool first_run = true;

	for(; std::regex_search(state->input, r_match,
	                        std::regex(REGEX_ADDR_PART)); first_run = false) {
		state->has_cmd_addr = true;

		std::string match = r_match.str();
		state->input = state->input.substr(match.length());

		// TODO switch case ??
		if (match.compare("$") == 0) {
			if (!first_run) {
				state->error = "Invalid address";

				return ADDR_PART_ERROR;
			}

			addr_offset = state->addr_offset_max;
		}
		else if (match.compare(".") == 0) {
			if (!first_run) {
				state->error = "Invalid address";

				return ADDR_PART_ERROR;
			}
		}
		else if (match.at(0) == '\'') {
			if (match.length() == 1) {
				state->error = "Invalid mark character";

				return ADDR_PART_ERROR;
			}

			int x = (int)match.at(1);

			if (islower(x) == 0) {
				state->error = "Invalid mark character";

				return ADDR_PART_ERROR;
			}

			auto marked_iter = state->addr_iter_marked[x - 'a'];

			if (marked_iter == state->buffer.end()) {
				state->error = "Invalid address";

				return ADDR_PART_ERROR;
			}

			addr_offset = 1 + std::distance(state->buffer.begin(),
			                                marked_iter);
		}
		else if (match.compare("/") == 0 || match.compare("?") == 0) {
			size_t i = 0;
			size_t ends = 0;

			// TODO regex
			while (i < state->input.length()) {
				if (state->input.at(i) == match.at(0)) {
					if (i >= 1 && state->input.at(i-1) == '\\') {
						i++;
						continue;
					}

					ends = 1;

					break;
				}

				i++;
			}

			std::string re;

			if (i == 0) {
				re = state->last_re;
			} else {
				re = state->input.substr(0, i);
				state->last_re = re;
				state->input = state->input.substr(i + ends);
			}

			if (re.length() == 0) {
				state->error = "No previous pattern";

				return ADDR_PART_ERROR;
			}

			std::regex re_pattern(re);
			std::smatch re_match;

			std::list<std::string>::iterator it;

			if (match.at(0) == '/') it = std::next(state->addr_iter_cur);
			else it = std::prev(state->addr_iter_cur); // == '?'

			auto end = it;

			do {
				if (it == state->buffer.end()) {
					if (match.at(0) == '/') it++;
					else it--; // == '?'

					continue;
				}

				if (std::regex_search(*it, re_match, re_pattern)) {
					addr_offset = 1 + std::distance(state->buffer.begin(), it);

					break;
				}

				if (match.at(0) == '/') it++;
				else it--; // == '?'
			} while (it != end);

			if (it == end) {
				state->error = "No match";

				return ADDR_PART_ERROR;
			}
		}
		else if (std::all_of(match.begin(), match.end(), ::isdigit)) {
			long number = std::stol(match);

			if (first_run) {
				addr_offset = number;
			} else {
				addr_offset += number;
			}
		}
		else if (match.compare("+") == 0) {
			addr_offset++;
		}
		else if (match.compare("-") == 0) {
			addr_offset--;
		}
		else if (match.length() >= 2) {
			long number = std::stol(match.substr(1));

			std::string token = match.substr(0, 1);

			if (token.compare("+") == 0) {
				addr_offset += number;
			}
			else if (token.compare("-") == 0) {
				addr_offset -= number;
			}
		}
	}

	if (first_run && r_match.length() == 0) {
		return ADDR_PART_NO_MATCH;
	}

	if (addr_offset > state->addr_offset_max || addr_offset < 0) {
		state->error = "Invalid address";

		return ADDR_PART_ERROR;
	}

	return addr_offset;
}
typedef enum {
	NONE,
	COMMA,
	SEMICOLON
} AddrSeperatorResult;

AddrSeperatorResult interpret_addr_seperator(ed_state *state)
{
	std::smatch match;

	AddrSeperatorResult seperator = NONE;

	if (std::regex_search(state->input, match,
	                      std::regex(REGEX_ADDR_SEPERATOR))) {
		if (match.str().compare(",") == 0) {
			seperator = COMMA;
		} else if (match.str().compare(";") == 0) {
			seperator = SEMICOLON;
		}

		state->input = state->input.substr(match.str().length());
		state->has_cmd_addr = true;
	}

	return seperator;
}

int interpret_addr(ed_state *state)
{
	AddrSeperatorResult seperator;

	state->addr_offset_max = std::distance(state->buffer.begin(),
	                                       state->buffer.end());

	state->num_addr = 0;

	ssize_t addr_offset_save = ADDR_PART_ERROR;

	do {
		ssize_t addr_offset = interpret_addr_part(state);

		if (addr_offset == ADDR_PART_ERROR) {
			return -1;
		}

		if (ADDR_PART_OK(addr_offset)) {
			if (state->num_addr == 1 || state->num_addr == 0) {
				state->num_addr++;
				state->addr_offset_begin = addr_offset;
				state->addr_offset_end = addr_offset;
			} else if (state->num_addr == 2) {
				state->num_addr++;
				state->addr_offset_end = addr_offset;
			} else if (state->num_addr > 2) {
				state->addr_offset_begin = state->addr_offset_end;
				state->addr_offset_end = addr_offset;
			}
		} else if (state->num_addr > 0 && addr_offset_save != ADDR_PART_ERROR) {
			state->num_addr = 2;

			state->addr_offset_begin = addr_offset_save;
		}

		addr_offset_save = ADDR_PART_ERROR;

		seperator = interpret_addr_seperator(state);

		if (state->num_addr == 0) {
			switch (seperator) {
				case NONE: {
					state->addr_offset_begin = state->addr_offset_cur;
					state->addr_offset_end = state->addr_offset_cur;
				} break;
				case COMMA: {
					state->addr_offset_begin = 1;
					state->addr_offset_end = state->addr_offset_max;

					state->num_addr = 2;
				} break;
				case SEMICOLON: {
					state->addr_offset_begin = state->addr_offset_cur;
					state->addr_offset_end = state->addr_offset_max;

					state->num_addr = 2;
				} break;
			}
		} else {
			switch (seperator) {
				case COMMA: {
					addr_offset_save = state->addr_offset_end;

					if (state->num_addr < 2) {
						state->num_addr = 2;
					}
				} break;
				case SEMICOLON: {
					state->addr_offset_cur = state->addr_offset_begin;
					addr_offset_save = state->addr_offset_end;

					if (state->num_addr < 2) {
						state->num_addr = 2;
					}
				} break;
				default: break;
			}
		}
	} while (seperator != NONE);

	if (state->num_addr > 2) {
		state->num_addr = 2;
	}

	return 0;
}

int validate_addr(ed_state *state, int expected_num_addr, bool allow_zero_addr)
{
	if (expected_num_addr == 0 && state->num_addr != 0) {
		state->error = "Unexpected address";
		return -1;
	}

	if (expected_num_addr == 1) {
		state->addr_offset_begin = state->addr_offset_end;
	}

	if (expected_num_addr > 0 && allow_zero_addr == false
		&& (state->addr_offset_begin == 0 || state->addr_offset_end == 0)) {
		state->error = "Invalid address";
		return -1;
	}

	if (
		(state->addr_offset_begin > state->addr_offset_max)
			||
		(state->addr_offset_end > state->addr_offset_max)
			||
		(state->addr_offset_begin > state->addr_offset_end)
	) {
		state->error = "Invalid address";
		return -1;
	}

	return 0;
}

int interpret_cmd(ed_state *state)
{
	state->cmd = '\0';
	state->params = std::smatch();
	state->suffix = '\0';

	if (state->input.length() == 0) {
		return 0;
	}

	std::smatch cmd_match;

	if (!std::regex_search(state->input, cmd_match, std::regex(REGEX_CMD))) {
		state->error = "Unknown command";
		return -1;
	} else if (cmd_match[1].str().length() == 0) {
		return 0;
	}

	state->cmd = cmd_match[1].str().at(0);

	state->input = state->input.substr(cmd_match.length());

	switch (state->cmd) {
		case 'e': case 'E': case 'f': {
			std::regex_search(state->input, state->params,
			                  std::regex(REGEX_PARAM_FILE));
		} break;
		case 'g': case 'G': case 'v': case 'V': {
			std::regex_search(state->input, state->params,
			                  std::regex(REGEX_PARAM_GLOBAL));
		} break;
		case 's': {
			std::regex_search(state->input, state->params,
			                  std::regex(REGEX_PARAM_SUBSTITUTE));
		} break;
		case 'k': {
			std::regex_search(state->input, state->params,
			                  std::regex(REGEX_PARAM_MARK));
		} break;
		case 'm': case 't': {
			std::regex_search(state->input, state->params,
			                  std::regex(REGEX_ADDR));
		} break;
		default: break;
	}

	std::string suffix_part = std::string(state->input);

	if (state->params.size() > 0) {
		 suffix_part = suffix_part.substr(state->params.length());
	}

	if (suffix_part.length() == 0) {
		return 0;
	}

	std::smatch suffix_match;

	std::regex_search(suffix_part, suffix_match, std::regex("^([lnp]|$)*"));

	switch (state->cmd) {
		case 'e': case 'E': case 'f': case 'q':
		case 'Q': case 'w': case '!': {
			if (suffix_match.length() != 0) {
				state->error = "Unexpected command suffix";
				return -1;
			}
		} break;
		default: {
			if (suffix_match.size() == 0) {
				state->error = "Invalid command suffix";
				return -1;
			} else if (suffix_part.length() != suffix_match.str().length()) {
				state->error = "Invalid command suffix";
				return -1;
			}

			state->suffix = suffix_match[0].str().at(0);
		} break;
	}

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
				print_usage(argv);
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

		state.input = "";

		if (state.prompt_print) {
			std::cout << state.promt << std::flush;
		}

		while (int ch = getchar()) {
			if (state.input.length() == 0 && ch == '\n') {
				state.input = "+1p";
				break;
			}

			if (ch == EOF || ch == '\n') {
				break;
			}

			state.input += ch;
		}

		if (state.input.length() == 0) {
			state.input = "q";
		}

		if (interpret_addr(&state) != 0) {
			ed_error(&state);

			continue;
		}

		if (interpret_cmd(&state) != 0) {
			ed_error(&state);

			continue;
		}

		if (run_command(&state) != 0) {
			ed_error(&state);

			continue;
		}

		if ((state.cmd = state.suffix) == '\0') {
			continue;
		}

		state.has_cmd_addr = false; // fallback is addr_offset_cur

		if (run_command(&state) != 0) {
			ed_error(&state);

			continue;
		}
	} while (state.runs);

	if (state.error.length() != 0) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
