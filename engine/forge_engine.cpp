// GEHU Forge engine - backend (Python) isse stdin/stdout pe baat karta hai.
// Ek line = ek command, jawab "END" tak.
//   PING                                   -> PONG
//   LOAD <tab> students.tsv <tab> achievements.tsv <tab> asOfIso
//                                          -> OK <students> <approved>
//   RANK                                   -> cat sid exp rank prevExp prevRank ... END
//   SEARCH <tab> k <tab> query             -> sid relevance ... END
//   QUIT
// Build: g++ -std=c++17 -O2 engine/forge_engine.cpp -o engine/forge_engine.exe
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "../RankEngine.h"
#include "../SearchEngine.h"

static const int NCAT = 6;
static const char* CAT_TYPE[NCAT] = {"CODING", "DSA", "OSS", "INTERN", "CERT", "ACAD"};
static const char* CAT_KEY[NCAT] = {"competitive", "dsa", "project", "internship", "certification", "academic"};

static void split(const std::string& line, char delim, DynArray<std::string>& out) {
    out.clear();
    std::string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == delim) { out.push_back(cur); cur.clear(); }
        else if (line[i] != '\r') cur += line[i];
    }
    out.push_back(cur);
}

static int catIndex(const std::string& type) {
    for (int i = 0; i < NCAT; ++i)
        if (type == CAT_TYPE[i]) return i;
    return -1;
}

struct Row {
    int id, studentId, score;
    std::string type, date, platform, reviewedAt;
};

// Ek "board set" = overall + 6 category boards. Abhi ka aur 7 din pehle ka alag banta hai.
struct Boards {
    RankEngine overall;
    RankEngine cat[NCAT];
};

struct State {
    DynArray<int> studentIds;
    HashMap<int, Student*> students;
    DynArray<Row> rows;
    Boards now, prev;
    SearchEngine search;
    DynArray<Achievement*> owned;   // Student inke pointers rakhta hai, isliye State ke saath hi delete

    ~State() {
        for (int i = 0; i < owned.size(); ++i) delete owned[i];
        students.forEach([](const int&, Student* const& s) { delete s; });
    }
};

static State* state = nullptr;

// Achievements ko boards pe apply karo. asOf khaali = sab, warna sirf us time tak reviewed.
static void build(Boards& b, const std::string& asOf, DynArray<Achievement*>& owned) {
    for (int i = 0; i < state->rows.size(); ++i) {
        const Row& r = state->rows[i];
        if (!asOf.empty() && r.reviewedAt > asOf) continue;
        Student** s = state->students.find(r.studentId);
        int ci = catIndex(r.type);
        if (s == nullptr || ci < 0) continue;
        Achievement* a = createAchievement(r.type, r.id, r.studentId, "x", r.date, r.score, "x", r.platform);
        if (a == nullptr) continue;
        owned.push_back(a);
        a->approve();
        double delta = 0.0;
        if (b.overall.applyAchievement(**s, *a, &delta)) b.cat[ci].addPoints(r.studentId, delta);
    }
}

static void cmdLoad(const DynArray<std::string>& f) {
    if (f.size() < 4) { std::printf("ERR usage\nEND\n"); return; }
    std::ifstream sf(f[1].c_str()), af(f[2].c_str());
    if (!sf || !af) { std::printf("ERR cannot open files\nEND\n"); return; }

    delete state;
    state = new State();
    std::string line;
    DynArray<std::string> c;

    // students.tsv: id name roll branch grad campus skills headline
    while (std::getline(sf, line)) {
        if (line.empty()) continue;
        split(line, '\t', c);
        if (c.size() < 8) continue;
        int id = std::atoi(c[0].c_str());
        state->studentIds.push_back(id);
        state->students.put(id, new Student(id, c[1], "-", c[3], std::atoi(c[4].c_str())));
    }
    // achievements.tsv (sirf approved, id order mein): id sid type date score platform reviewedAt
    while (std::getline(af, line)) {
        if (line.empty()) continue;
        split(line, '\t', c);
        if (c.size() < 7) continue;
        Row r;
        r.id = std::atoi(c[0].c_str());
        r.studentId = std::atoi(c[1].c_str());
        r.type = c[2];
        r.date = c[3];
        r.score = std::atoi(c[4].c_str());
        r.platform = c[5];
        r.reviewedAt = c[6];
        state->rows.push_back(r);
    }

    build(state->now, "", state->owned);
    build(state->prev, f[3], state->owned);

    // Search index EXP ke saath (tie-break ke liye)
    sf.clear();
    sf.seekg(0);
    while (std::getline(sf, line)) {
        if (line.empty()) continue;
        split(line, '\t', c);
        if (c.size() < 8) continue;
        int id = std::atoi(c[0].c_str());
        state->search.addStudent(id, c[1], c[2], c[3], c[6], c[7], state->now.overall.getExp(id));
    }
    std::printf("OK %d %d\nEND\n", state->studentIds.size(), state->rows.size());
}

static void printBoard(const char* key, RankEngine& nowB, RankEngine& prevB) {
    for (int i = 0; i < state->studentIds.size(); ++i) {
        int sid = state->studentIds[i];
        int rank = nowB.getRank(sid);
        if (rank < 0) continue;
        std::printf("%s\t%d\t%.4f\t%d\t%.4f\t%d\n", key, sid, nowB.getExp(sid), rank,
                    prevB.getExp(sid), prevB.getRank(sid));
    }
}

static void cmdRank() {
    if (state == nullptr) { std::printf("ERR not loaded\nEND\n"); return; }
    printBoard("all", state->now.overall, state->prev.overall);
    for (int i = 0; i < NCAT; ++i) printBoard(CAT_KEY[i], state->now.cat[i], state->prev.cat[i]);
    std::printf("END\n");
}

static void cmdSearch(const DynArray<std::string>& f) {
    if (state == nullptr || f.size() < 3) { std::printf("ERR not loaded\nEND\n"); return; }
    int k = std::atoi(f[1].c_str());
    DynArray<SearchHit> hits;
    state->search.search(f[2], k, hits);
    for (int i = 0; i < hits.size(); ++i) std::printf("%d\t%d\n", hits[i].id, hits[i].relevance);
    std::printf("END\n");
}

int main() {
    std::string line;
    DynArray<std::string> f;
    while (std::getline(std::cin, line)) {
        split(line, '\t', f);
        const std::string& cmd = f[0];
        if (cmd == "PING") std::printf("PONG\nEND\n");
        else if (cmd == "LOAD") cmdLoad(f);
        else if (cmd == "RANK") cmdRank();
        else if (cmd == "SEARCH") cmdSearch(f);
        else if (cmd == "QUIT") break;
        else std::printf("ERR unknown command\nEND\n");
        std::fflush(stdout);
    }
    delete state;
    return 0;
}
