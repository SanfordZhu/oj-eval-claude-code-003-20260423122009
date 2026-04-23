#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <queue>

using namespace std;

struct ProblemInfo {
    bool solved;
    int solved_time;
    int wrong_attempts;
    int submissions_after_freeze;

    ProblemInfo() : solved(false), solved_time(0), wrong_attempts(0), submissions_after_freeze(0) {}
};

struct Team {
    string name;
    int solved_problems;
    int total_penalty;
    map<char, ProblemInfo> problems;
    vector<int> solve_times;

    Team() : solved_problems(0), total_penalty(0) {}

    // For sorting
    bool operator<(const Team& other) const {
        if (solved_problems != other.solved_problems)
            return solved_problems > other.solved_problems;
        if (total_penalty != other.total_penalty)
            return total_penalty < other.total_penalty;

        // Compare solve times in descending order
        for (int i = (int)solve_times.size() - 1; i >= 0; i--) {
            if (i >= (int)other.solve_times.size()) return true;
            if (solve_times[i] != other.solve_times[i])
                return solve_times[i] < other.solve_times[i];
        }
        if (solve_times.size() != other.solve_times.size())
            return solve_times.size() > other.solve_times.size();

        return name < other.name;
    }
};

struct Submission {
    string team;
    char problem;
    string status;
    int time;

    Submission(string t, char p, string s, int ti)
        : team(t), problem(p), status(s), time(ti) {}
};

class ICPCManager {
private:
    map<string, Team> teams;
    vector<string> team_order;  // Order of teams added
    bool competition_started;
    bool frozen;
    int duration_time;
    int problem_count;
    vector<Submission> all_submissions;

    // For scroll operation
    vector<pair<string, char>> frozen_queue;

    vector<Team> getSortedTeams() {
        vector<Team> sorted_teams;
        for (const auto& p : teams) {
            sorted_teams.push_back(p.second);
        }
        sort(sorted_teams.begin(), sorted_teams.end());
        return sorted_teams;
    }

    int getTeamRank(const string& team_name) {
        vector<Team> sorted = getSortedTeams();
        for (int i = 0; i < sorted.size(); i++) {
            if (sorted[i].name == team_name) {
                return i + 1;
            }
        }
        return -1;
    }

    void printScoreboard(const vector<Team>& sorted_teams) {
        for (int i = 0; i < sorted_teams.size(); i++) {
            const Team& team = sorted_teams[i];
            cout << team.name << " " << (i + 1) << " " << team.solved_problems
                 << " " << team.total_penalty;

            // Print problem status
            for (char p = 'A'; p < 'A' + problem_count; p++) {
                cout << " ";
                const ProblemInfo& info = team.problems.at(p);

                if (frozen && !info.solved && info.submissions_after_freeze > 0) {
                    // Frozen problem
                    if (info.wrong_attempts == 0) {
                        cout << "0/" << info.submissions_after_freeze;
                    } else {
                        cout << "-" << info.wrong_attempts << "/" << info.submissions_after_freeze;
                    }
                } else {
                    // Not frozen
                    if (info.solved) {
                        if (info.wrong_attempts == 0) {
                            cout << "+";
                        } else {
                            cout << "+" << info.wrong_attempts;
                        }
                    } else {
                        if (info.wrong_attempts == 0) {
                            cout << ".";
                        } else {
                            cout << "-" << info.wrong_attempts;
                        }
                    }
                }
            }
            cout << "\n";
        }
    }

public:
    ICPCManager() : competition_started(false), frozen(false), duration_time(0), problem_count(0) {}

    void addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.count(team_name)) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        Team team;
        team.name = team_name;
        teams[team_name] = team;
        team_order.push_back(team_name);
        cout << "[Info]Add successfully.\n";
    }

    void startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }

        competition_started = true;
        duration_time = duration;
        problem_count = problems;

        // Initialize problems for all teams
        for (auto& p : teams) {
            for (char c = 'A'; c < 'A' + problem_count; c++) {
                p.second.problems[c] = ProblemInfo();
            }
        }

        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& team_name,
                const string& status, int time) {
        if (!teams.count(team_name)) return;

        char prob = problem[0];
        Team& team = teams[team_name];
        ProblemInfo& info = team.problems[prob];

        // Record submission
        all_submissions.emplace_back(team_name, prob, status, time);

        if (!info.solved) {
            if (frozen) {
                info.submissions_after_freeze++;
                frozen_queue.emplace_back(team_name, prob);
            }

            if (status == "Accepted") {
                info.solved = true;
                info.solved_time = time;
                team.solved_problems++;
                team.total_penalty += 20 * info.wrong_attempts + time;
                team.solve_times.push_back(time);
                sort(team.solve_times.begin(), team.solve_times.end());
            } else {
                info.wrong_attempts++;
            }
        }
    }

    void flush() {
        cout << "[Info]Flush scoreboard.\n";
    }

    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        frozen_queue.clear();
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // Print scoreboard before scrolling (after flush)
        vector<Team> before_teams = getSortedTeams();
        printScoreboard(before_teams);

        // Process unfreezing
        while (!frozen_queue.empty()) {
            // Build current frozen state
            map<string, set<char>> team_frozen_problems;
            for (const auto& p : frozen_queue) {
                team_frozen_problems[p.first].insert(p.second);
            }

            // Find lowest ranked team with frozen problems
            vector<Team> current_ranking = getSortedTeams();
            string target_team;
            char target_problem = 0;

            for (int i = current_ranking.size() - 1; i >= 0; i--) {
                const Team& team = current_ranking[i];
                if (team_frozen_problems.count(team.name)) {
                    target_team = team.name;
                    target_problem = *team_frozen_problems[team.name].begin();
                    break;
                }
            }

            if (target_team.empty()) break;

            // Unfreeze this problem
            Team& team = teams[target_team];
            ProblemInfo& info = team.problems[target_problem];
            info.submissions_after_freeze = 0;

            // Remove from frozen queue
            auto it = find(frozen_queue.begin(), frozen_queue.end(),
                          make_pair(target_team, target_problem));
            if (it != frozen_queue.end()) {
                frozen_queue.erase(it);
            }

            // Check if ranking changed
            vector<Team> new_ranking = getSortedTeams();

            // Find old and new rank of the team
            int old_rank = -1, new_rank = -1;
            for (int i = 0; i < current_ranking.size(); i++) {
                if (current_ranking[i].name == target_team) {
                    old_rank = i + 1;
                    break;
                }
            }
            for (int i = 0; i < new_ranking.size(); i++) {
                if (new_ranking[i].name == target_team) {
                    new_rank = i + 1;
                    break;
                }
            }

            // Output if ranking changed
            if (old_rank != new_rank) {
                string replaced_team;
                if (new_rank < old_rank) {
                    // Moved up - replaced the team at new_rank
                    replaced_team = current_ranking[new_rank - 1].name;
                } else {
                    // Moved down - replaced the team at old_rank
                    replaced_team = current_ranking[old_rank - 1].name;
                }

                cout << target_team << " " << replaced_team << " "
                     << team.solved_problems << " " << team.total_penalty << "\n";
            }
        }

        // Print final scoreboard
        vector<Team> final_teams = getSortedTeams();
        printScoreboard(final_teams);

        frozen = false;
    }

    void queryRanking(const string& team_name) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        int rank = getTeamRank(team_name);
        cout << team_name << " NOW AT RANKING " << rank << "\n";
    }

    void querySubmission(const string& team_name, const string& problem,
                        const string& status) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        // Find last matching submission
        Submission* last_match = nullptr;
        for (auto it = all_submissions.rbegin(); it != all_submissions.rend(); ++it) {
            if (it->team != team_name) continue;
            if (problem != "ALL" && it->problem != problem[0]) continue;
            if (status != "ALL" && it->status != status) continue;
            last_match = &(*it);
            break;
        }

        if (!last_match) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << last_match->team << " " << last_match->problem << " "
                 << last_match->status << " " << last_match->time << "\n";
        }
    }

    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ICPCManager manager;
    string line;

    while (getline(cin, line)) {
        if (line == "END") {
            manager.end();
            break;
        }

        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            manager.addTeam(team_name);
        } else if (cmd == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            manager.startCompetition(duration, problems);
        } else if (cmd == "SUBMIT") {
            string problem, dummy, team_name, dummy2, status, dummy3;
            int time;
            iss >> problem >> dummy >> team_name >> dummy2 >> status >> dummy3 >> time;
            manager.submit(problem, team_name, status, time);
        } else if (cmd == "FLUSH") {
            manager.flush();
        } else if (cmd == "FREEZE") {
            manager.freeze();
        } else if (cmd == "SCROLL") {
            manager.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            manager.queryRanking(team_name);
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name, dummy, problem, dummy2, status;
            iss >> team_name >> dummy >> problem >> dummy2 >> status;
            manager.querySubmission(team_name, problem, status);
        }
    }

    return 0;
}