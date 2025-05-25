#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <regex>
#include <algorithm>
#include <cctype>

int main() {
	std::list<std::string> lines;

	std::string filename = "src/ed++.cpp";

	{
		std::ifstream file(filename);

		size_t nread = 0;

		for (std::string line; std::getline(file, line);) {
			lines.push_back(line);

			nread += line.length() + 1;
		}

		std::cout << nread << "\n";
	}

	filename = "src/ed++.cpp.2";

	bool printPrompt = false;
	std::string promt = "*";

	std::list<std::string>::iterator addr_iter_current = std::prev(lines.end());

	while (1) {
		std::string cmd;

		if (printPrompt) {
			std::cout << promt;
		}

		std::getline(std::cin, cmd);

		std::regex address_pattern("^([0-9])+|[$]|[.]");

		std::smatch address_start_match;

		if (std::regex_search(cmd, address_start_match, address_pattern)) {
//			std::cout << "Match found: " << address_start_match.str() << std::endl;
//			std::cout << "Length: " << address_start_match.length() << std::endl;
		} else {
//			std::cout << "No match found (start)." << std::endl;
		}

		std::regex address_seperator_pattern("^[,]|[;]");
		std::smatch address_seperator_match;

		std::string cmd_sep = cmd.substr(address_start_match.length());

		if (std::regex_search(cmd_sep, address_seperator_match, address_seperator_pattern)) {
//			std::cout << "Match found: " << address_seperator_match.str() << std::endl;
//			std::cout << "Length: " << address_seperator_match.length() << std::endl;
		} else {
//			std::cout << "No match found (seperator)." << std::endl;
		}

		std::smatch address_end_match;

		std::string cmd_end = cmd_sep.substr(address_seperator_match.length());

		if (std::regex_search(cmd_end, address_end_match, address_pattern)) {
//			std::cout << "Match found: " << address_end_match.str() << std::endl;
//			std::cout << "Length: " << address_end_match.length() << std::endl;
		} else {
//			std::cout << "No match found (end)." << std::endl;
		}

		std::regex command_pattern("^[q]|[p]|[n]|[d]|[P]|[w]|[a]|[s]|[g]|[i]");
		std::smatch command_match;

		std::string cmd_com = cmd_end.substr(address_end_match.length());

		if (std::regex_search(cmd_com, command_match, command_pattern)) {
//			std::cout << "Match found: " << command_match.str() << std::endl;
//			std::cout << "Length: " << command_match.length() << std::endl;
		} else {
//			std::cout << "No match found (command)." << std::endl;
		}

		std::list<std::string>::iterator addr_iter_begin;
		std::list<std::string>::iterator addr_iter_end;

		if (address_start_match.length() == 0 && address_end_match.length() == 0) {
			if (address_seperator_match.length() == 0) {
				addr_iter_begin = addr_iter_current;
				addr_iter_end = addr_iter_current;
			}
			else {
				std::string match = address_seperator_match.str();

				if (match.compare(",") == 0) {
					addr_iter_begin = lines.begin();
					addr_iter_end = std::prev(lines.end());
				}
				else if (match.compare(";") == 0) {
					addr_iter_begin = addr_iter_current;
					addr_iter_end = std::prev(lines.end());
				}
			}
		}

		if (address_start_match.length() > 0) {
			std::string match = address_start_match.str();

			if (match.compare("$") == 0) {
				addr_iter_begin = std::prev(lines.end());
			}
			else if (match.compare(".") == 0) {
				addr_iter_begin = addr_iter_current;
			}
			else if (std::all_of(match.begin(), match.end(), ::isdigit)) {
				addr_iter_begin = std::next(lines.begin(), std::stoi(match) - 1);
			}
			else {
				std::cout << "Error interpreting (start): " << match << std::endl;
			}
		}

		if (address_end_match.length() > 0) {
			std::string match = address_end_match.str();

			if (match.compare("$") == 0) {
				addr_iter_end = std::prev(lines.end());
			}
			else if (match.compare(".") == 0) {
				addr_iter_end = addr_iter_current;
			}
			else if (std::all_of(match.begin(), match.end(), ::isdigit)) {
				addr_iter_end = std::next(lines.begin(), std::stoi(match) - 1);
			}
			else {
				std::cout << "Error interpreting (end)" << std::endl;
			}
		}
		else if (address_start_match.length() > 0) {
			addr_iter_end = addr_iter_begin;
		}

		if (command_match.length() > 0) {
			std::string match = command_match.str();

			if (match.compare("q") == 0) {
				return 0;
			}
			else if (match.compare("p") == 0 || match.compare("n") == 0) {
				bool line_numbers = match.compare("n") == 0;

				int line = std::distance(lines.begin(), addr_iter_begin) + 1;

				for (auto it = addr_iter_begin; it != std::next(addr_iter_end); it++) {
					if (line_numbers) {
						std::cout << line << "\t" << *it << "\n";
					}
					else {
						std::cout << *it << "\n";
					}

					line++;
				}
			}
			else if (match.compare("d") == 0) {
				auto addr_iter_temp = std::next(addr_iter_end);

				if (addr_iter_temp == std::prev(lines.end()) || addr_iter_temp == lines.end()) {
					addr_iter_temp = std::prev(addr_iter_begin);
				}

				auto end = std::next(addr_iter_end);

				for (auto it = addr_iter_begin; it != end; ) {
					it = lines.erase(it);
				}

				addr_iter_end = addr_iter_temp;
			}
			else if (match.compare("P") == 0) {
				printPrompt = !printPrompt;
			}
			else if (match.compare("w") == 0) {
				std::ofstream outfile(filename);

				for (auto it = lines.begin(); it != lines.end(); it++) {
					outfile << *it << "\n";
				}

				std::cout << outfile.tellp() << "\n";
			}
			else if (match.compare("a") == 0) {
				addr_iter_end = addr_iter_begin;

				while (1) {
					std::string line;

					std::getline(std::cin, line);

					if (line.compare(".") == 0)
					{
						break;
					}

					lines.insert(std::next(addr_iter_end), line);

					addr_iter_end = std::next(addr_iter_end);
				}
			}
			else if (match.compare("i") == 0) {
				addr_iter_end = addr_iter_begin;

				while (1) {
					std::string line;

					std::getline(std::cin, line);

					if (line.compare(".") == 0)
					{
						break;
					}

					lines.insert(addr_iter_begin, line);

					addr_iter_end = std::prev(addr_iter_begin);
				}
			}
			else if (match.compare("s") == 0) {
				std::string parameters = cmd_com.substr(command_match.length());

				std::regex pattern("^/([^/]+)/([^/]+)/");
				std::smatch matches;

				std::regex_search(parameters, matches, pattern);

				if (matches.size() >= 2) {
					std::string rsp = matches[1];
					std::string rpl = matches[2];
					std::regex regex_search_pattern(rsp);

					for (auto it = addr_iter_begin; it != std::next(addr_iter_end); it++) {
						*it = std::regex_replace(*it, regex_search_pattern, rpl);
					}
				}
				else {
					std::cout << "TODO: regex error not matching!\n";
				}
			}
			else if (match.compare("g") == 0) {
				std::string parameters = cmd_com.substr(command_match.length());

				std::regex pattern("^/([^/]+)/[np]");
				std::smatch matches;

				std::regex_search(parameters, matches, pattern);

				if (matches.size() >= 2) {
					std::string sp = matches[1];
					std::string gc = matches[2];
					std::regex regex_search_pattern(sp);
					std::smatch match;

					for (auto it = addr_iter_begin; it != std::next(addr_iter_end); it++) {
						// TODO eval command: gc string to run the correct comamnd and not
						// just print like function
						if (std::regex_search(*it, match, regex_search_pattern)) {
							std::string cs = std::string(*it);

							size_t pos = 0;

							for (size_t i = 0; i < match.size(); i++) {
								size_t npos = cs.substr(pos).find(match[i].str());

								if (npos != std::string::npos) {
									std::cout << cs.substr(pos, npos) <<
											"\033[31m" <<
									cs.substr(pos + npos, match[i].str().length()) <<
											"\033[0m";
									pos += npos + match[i].str().length();
								}
							}
							std::cout << cs.substr(pos) << "\n";
						}
					}
				}
				else {
					std::cout << "TODO: regex error not matching!\n";
				}
			}
		}


		addr_iter_current = addr_iter_end;
	}

	return 0;
}
