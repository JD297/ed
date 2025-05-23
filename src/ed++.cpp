#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <regex>
#include <algorithm>
#include <cctype>

int main() {
	std::ifstream file("src/ed++.cpp");
	std::list<std::string> lines;

	for (std::string line; std::getline(file, line);) {
		lines.push_back(line);
	}

	std::list<std::string>::iterator addr_iter_current = std::prev(lines.end());

	while (1) {
		std::string cmd;

		std::cout << "*";

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

		std::regex command_pattern("^[q]|[p]|[n]|[d]");
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

				for (auto it = addr_iter_begin; it != std::next(addr_iter_end); it++) {
					lines.erase(it);
				}

				addr_iter_end = addr_iter_temp;
			}
		}

		addr_iter_current = addr_iter_end;
	}

	return 0;
}
