#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <set>

int count_occurrence(const std::string& heystack, const std::string& needle)
{
	int cnt = 0;
	for(int s=0; s+needle.size()<=heystack.size(); ++s)
		if(needle == heystack.substr(s, needle.size()))
			++cnt;
	return cnt;
}

int score(const std::string& cmd, const std::vector<std::string>& phrases)
{
	int total_score = 0;
	for(auto& p: phrases) {
		int resp = count_occurrence(cmd, p);
		int lenp = p.size();
		total_score += 2*lenp*resp + (resp>0 ? 300 : 0);
	}
	return total_score;
}

char canonical(char c)
{
	std::string group[] = {
		"p'!.03",
		"bcefy2",
		"aghij4",
		"lmno 5",
		"dqrvz1",
		"kstuwx",
	};
	for(int i=0; i<6; ++i)
		if(group[i].find(c) != std::string::npos)
			return group[i][0];
	return c;
}

bool match(char c1, char c2)
{
	return canonical(c1) == canonical(c2);
}

bool match(const std::string& base, int s, const std::string& target)
{
	for(int i=0; i<target.size(); ++i)
		if(s+i>=base.size() || !match(base[s+i], target[i]))
			return false;
	return true;
}

std::string solve(const std::string& cmd, const std::vector<std::string>& phrases)
{
	std::cerr<< "Before: " << score(cmd, phrases) << std::endl;

	// simple greedy changer.
	// TODO dp-based changer.
	std::set<std::string> p1(phrases.begin(), phrases.end()), p2;
	std::string result;
	while(result.size() < cmd.size()) {
		bool done = false;
		if(!done)
			for(auto p: p1)
				if(match(cmd,result.size(),p)) {
					done = true;
					result += p;
					p2.insert(p);
					p1.erase(p);
					break;
				}
		if(!done)
			for(auto p: p2)
				if(match(cmd,result.size(),p)) {
					done = true;
					result += p;
					break;
				}
		if(!done)
			result += cmd[result.size()];
	}

	std::cerr<< "After: " << score(result, phrases) << std::endl;
	return result;
}

int main(int argc, const char* argv[])
{
	std::vector<std::string> phrases;
	for(int i=0; i+1<argc; ++i) {
		if(argv[i] == std::string("-p"))
			phrases.emplace_back(argv[++i]);
	}

	std::string cmd;
	std::getline(std::cin, cmd);
	std::cout << solve(cmd, phrases) << std::endl;
}
