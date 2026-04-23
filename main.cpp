#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <queue>
#include <cstring>

using namespace std;

const int MAX_TEAMS = 10000;
const int MAX_PROBLEMS = 26;

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
    ProblemInfo problems[MAX_PROBLEMS];
    vector<int> solve_times;
    int rank;

    Team() : solved_problems(0), total_penalty(0), rank(0) {
        for (int i = 0; i < MAX_PROBLEMS; i++) {
            problems[i] = ProblemInfo();
        }
    }

    void clear() {
        solved_problems = 0;
        total_penalty = 0;
        rank = 0;
        solve_times.clear();
        for (int i = 0; i < MAX_PROBLEMS; i++) {
            problems[i] = ProblemInfo();
        }
    }
};

struct Submission {
    string team;
    char problem;
    string status;
    int time;

    Submission() : problem(0), time(0) {}
    Submission(string t, char p, string s, int ti)
        : team(t), problem(p), status(s), time(ti) {}
};

// Fast string comparison for team names
struct TeamComparator {
    bool operator()(const Team* a, const Team* b) const {
        if (a->solved_problems != b->solved_problems)
            return a->solved_problems > b->solved_problems;
        if (a->total_penalty != b->total_penalty)
            return a->total_penalty < b->total_penalty;

        // Compare solve times in descending order
        int a_size = a->solve_times.size();
        int b_size = b->solve_times.size();
        for (int i = a_size - 1, j = b_size - 1; i >= 0 && j >= 0; i--, j--) {
            if (a->solve_times[i] != b->solve_times[j])
                return a->solve_times[i] < b->solve_times[j];
        }
        if (a_size != b_size)
            return a_size > b_size;

        return a->name < b->name;
    }
};

class ICPCManager {
private:
    Team teams[MAX_TEAMS];
    int team_count;
    map<string, int> team_index;
    bool competition_started;
    bool frozen;
    int duration_time;
    int problem_count;
    vector<Submission> all_submissions;
    vector<Team*> sorted_teams;
    bool has_flushed;  // Track if any flush has occurred

    void updateRanking() {
        sorted_teams.clear();
        for (int i = 0; i < team_count; i++) {
            sorted_teams.push_back(&teams[i]);
        }

        // Before first flush, sort by lexicographic order
        if (!has_flushed) {
            sort(sorted_teams.begin(), sorted_teams.end(),
                [](const Team* a, const Team* b) { return a->name < b->name; });
        } else {
            sort(sorted_teams.begin(), sorted_teams.end(), TeamComparator());
        }

        // Update ranks
        for (int i = 0; i < sorted_teams.size(); i++) {
            sorted_teams[i]->rank = i + 1;
        }
    }

    int getTeamRank(const string& team_name) {
        if (!team_index.count(team_name)) return -1;
        return teams[team_index[team_name]].rank;
    }

    void printScoreboard() {
        for (int i = 0; i < sorted_teams.size(); i++) {
            const Team& team = *sorted_teams[i];
            cout << team.name << " " << (i + 1) << " " << team.solved_problems
                 << " " << team.total_penalty;

            // Print problem status
            for (int p = 0; p < problem_count; p++) {
                cout << " ";
                const ProblemInfo& info = team.problems[p];

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
    ICPCManager() : team_count(0), competition_started(false),
                   frozen(false), duration_time(0), problem_count(0), has_flushed(false) {}

    void addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (team_index.count(team_name)) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        teams[team_count].name = team_name;
        teams[team_count].clear();
        team_index[team_name] = team_count;
        team_count++;
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

        // Initialize initial ranking (lexicographic)
        updateRanking();

        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& team_name,
                const string& status, int time) {
        if (!team_index.count(team_name)) return;

        int idx = team_index[team_name];
        Team& team = teams[idx];
        int prob = problem[0] - 'A';
        ProblemInfo& info = team.problems[prob];

        // Record submission
        all_submissions.emplace_back(team_name, problem[0], status, time);

        if (!info.solved) {
            if (frozen) {
                info.submissions_after_freeze++;
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
        has_flushed = true;
        updateRanking();
        cout << "[Info]Flush scoreboard.\n";
        printScoreboard();
    }

    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // Update ranking before scrolling
        updateRanking();

        // Print scoreboard before scrolling
        printScoreboard();

        // Process unfreezing
        while (true) {
            // Build current frozen state
            map<string, vector<char>> team_frozen_problems;

            // Collect all frozen problems
            for (int i = 0; i < team_count; i++) {
                const Team& team = teams[i];
                for (int p = 0; p < problem_count; p++) {
                    if (!team.problems[p].solved && team.problems[p].submissions_after_freeze > 0) {
                        team_frozen_problems[team.name].push_back('A' + p);
                    }
                }
            }

            if (team_frozen_problems.empty()) break;

            // Find lowest ranked team with frozen problems
            string target_team;
            char target_problem = 0;

            for (int i = sorted_teams.size() - 1; i >= 0; i--) {
                const Team& team = *sorted_teams[i];
                if (team_frozen_problems.count(team.name)) {
                    target_team = team.name;
                    // Find smallest problem letter
                    target_problem = *min_element(team_frozen_problems[team.name].begin(),
                                                 team_frozen_problems[team.name].end());
                    break;
                }
            }

            if (target_team.empty()) break;

            // Unfreeze this problem
            int team_idx = team_index[target_team];
            Team& team = teams[team_idx];
            int prob_idx = target_problem - 'A';
            ProblemInfo& info = team.problems[prob_idx];
            info.submissions_after_freeze = 0;

            // Store old ranking
            vector<string> old_order;
            for (Team* t : sorted_teams) {
                old_order.push_back(t->name);
            }

            // Update ranking
            updateRanking();

            // Find if this team's ranking changed
            int old_rank = -1, new_rank = -1;
            for (int i = 0; i < old_order.size(); i++) {
                if (old_order[i] == target_team) {
                    old_rank = i + 1;
                    break;
                }
            }
            for (int i = 0; i < sorted_teams.size(); i++) {
                if (sorted_teams[i]->name == target_team) {
                    new_rank = i + 1;
                    break;
                }
            }

            // Output if ranking changed
            if (old_rank != new_rank) {
                string replaced_team;
                if (new_rank < old_rank) {
                    // Moved up - replaced the team at new_rank
                    replaced_team = old_order[new_rank - 1];
                } else {
                    // Moved down - replaced the team at old_rank
                    replaced_team = old_order[old_rank - 1];
                }

                cout << target_team << " " << replaced_team << " "
                     << team.solved_problems << " " << team.total_penalty << "\n";
            }
        }

        // Print final scoreboard
        printScoreboard();

        frozen = false;
    }

    void queryRanking(const string& team_name) {
        if (!team_index.count(team_name)) {
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
        if (!team_index.count(team_name)) {
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
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);

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