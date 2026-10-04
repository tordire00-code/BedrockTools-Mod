// Examiner - in-game explorer for libminecraftpe.so (aarch64 Android).
// Loaded as a plain .so: the constructor starts a worker thread that waits for the game
// library, then (re)runs the commands in input.txt whenever the file changes, so you can
// edit commands while the game keeps running. Original code, built on public formats only
// (ELF program headers, Itanium C++ RTTI/vtable layout, ARM64 instruction encodings).
#include <cxxabi.h>
#include <link.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace ex {

struct Seg { uintptr_t s, e; bool r, w, x; };
struct Lib { uintptr_t bias = 0; std::vector<Seg> segs; bool ok = false; };

static std::string fmt(const char* f, ...) {
    char buf[1024];
    va_list a; va_start(a, f); vsnprintf(buf, sizeof buf, f, a); va_end(a);
    return buf;
}

// ---- library discovery (live memory) ----------------------------------------------------
static Lib findLib(const char* needle) {
    struct Ctx { const char* n; Lib lib; } c{needle, {}};
    dl_iterate_phdr([](dl_phdr_info* i, size_t, void* p) -> int {
        auto* c = static_cast<Ctx*>(p);
        if (!i->dlpi_name || !strstr(i->dlpi_name, c->n)) return 0;
        c->lib.bias = i->dlpi_addr;
        for (int k = 0; k < i->dlpi_phnum; ++k) {
            const auto& h = i->dlpi_phdr[k];
            if (h.p_type != PT_LOAD) continue;
            uintptr_t s = i->dlpi_addr + h.p_vaddr;
            c->lib.segs.push_back({s, s + h.p_memsz, (h.p_flags & PF_R) != 0, (h.p_flags & PF_W) != 0, (h.p_flags & PF_X) != 0});
        }
        c->lib.ok = true;
        return 1;
    }, &c);
    return c.lib;
}

static bool readable(const Lib& L, uintptr_t p) { for (auto& g : L.segs) if (g.r && p >= g.s && p < g.e) return true; return false; }
static bool inExec(const Lib& L, uintptr_t p) { for (auto& g : L.segs) if (g.x && p >= g.s && p < g.e) return true; return false; }

// ---- scanning ----------------------------------------------------------------------------
// pattern bytes: 0..255, or -1 for a wildcard
static std::vector<uintptr_t> scan(const Lib& L, const std::vector<int>& pat, bool execOnly, bool dataOnly, size_t maxHits) {
    std::vector<uintptr_t> hits;
    const size_t n = pat.size();
    if (!n) return hits;
    size_t first = 0;
    while (first < n && pat[first] < 0) ++first;
    for (auto& g : L.segs) {
        if (!g.r || (execOnly && !g.x) || (dataOnly && g.x)) continue;
        const auto* b = reinterpret_cast<const uint8_t*>(g.s);
        const size_t len = g.e - g.s;
        if (len < n) continue;
        for (size_t i = 0; i + n <= len; ++i) {
            if (first < n && b[i + first] != static_cast<uint8_t>(pat[first])) continue;
            size_t k = 0;
            while (k < n && (pat[k] < 0 || b[i + k] == static_cast<uint8_t>(pat[k]))) ++k;
            if (k == n) { hits.push_back(g.s + i); if (hits.size() >= maxHits) return hits; }
        }
    }
    return hits;
}

// 8-byte aligned pointer search in readable, non-executable segments
static std::vector<uintptr_t> findPtr(const Lib& L, uint64_t v, size_t maxHits = 64) {
    std::vector<uintptr_t> out;
    for (auto& g : L.segs) {
        if (!g.r || g.x) continue;
        for (uintptr_t p = (g.s + 7) & ~uintptr_t(7); p + 8 <= g.e; p += 8) {
            if (*reinterpret_cast<const uint64_t*>(p) == v) { out.push_back(p); if (out.size() >= maxHits) return out; }
        }
    }
    return out;
}

// ---- RTTI names / vtables ----------------------------------------------------------------
struct Name { uintptr_t addr; std::string raw; };

static std::vector<Name> findNames(const Lib& L, const std::string& sub, size_t maxNames) {
    std::vector<int> pat;
    for (unsigned char c : sub) pat.push_back(c);
    std::set<uintptr_t> starts;
    std::vector<Name> out;
    for (uintptr_t h : scan(L, pat, false, true, 4000)) {
        uintptr_t s = h;
        int back = 0;
        while (back < 4096 && readable(L, s - 1) && *reinterpret_cast<const uint8_t*>(s - 1) != 0) { --s; ++back; }
        if (back >= 4096 || !starts.insert(s).second) continue;
        // an RTTI type name is referenced by a typeinfo object's name pointer
        if (findPtr(L, s, 1).empty()) continue;
        out.push_back({s, std::string(reinterpret_cast<const char*>(s), strnlen(reinterpret_cast<const char*>(s), 4096))});
        if (out.size() >= maxNames) break;
    }
    return out;
}

static std::string demangleType(const std::string& raw) {
    int st = 0;
    char* d = abi::__cxa_demangle(("_ZTS" + raw).c_str(), nullptr, nullptr, &st);
    std::string r = (st == 0 && d) ? d : raw;
    free(d);
    const std::string pre = "typeinfo name for ";
    if (r.rfind(pre, 0) == 0) r = r.substr(pre.size());
    return r;
}

static void dumpVtables(std::string& o, const Lib& L, const Name& n, int maxSlots) {
    for (uintptr_t loc : findPtr(L, n.addr, 8)) {          // typeinfo = { vptr, name*, ... }
        const uint64_t ti = loc - 8;
        auto refs = findPtr(L, ti, 64);                    // vtable[-1] == typeinfo*, or a derived typeinfo's base pointer
        std::vector<uintptr_t> vts;
        for (uintptr_t r : refs) {                         // a real vtable is followed by a code pointer
            if (readable(L, r + 8) && inExec(L, *reinterpret_cast<const uint64_t*>(r + 8))) vts.push_back(r);
        }
        o += fmt("  typeinfo rva=0x%llx, %zu vtable(s), %zu other ref(s)\n", (unsigned long long)(ti - L.bias), vts.size(), refs.size() - vts.size());
        for (uintptr_t vt : vts) {
            o += fmt("    vtable rva=0x%llx\n", (unsigned long long)(vt + 8 - L.bias));
            for (int i = 0; i < maxSlots; ++i) {
                uintptr_t slot = vt + 8 + 8 * i;
                if (!readable(L, slot)) break;
                uint64_t f = *reinterpret_cast<const uint64_t*>(slot);
                if (!inExec(L, f)) break;
                o += fmt("      [%d] rva=0x%llx\n", i, (unsigned long long)(f - L.bias));
            }
        }
    }
}

// ---- ARM64 helpers (pure functions, testable anywhere) -----------------------------------
// instructions that load a 32-bit constant lo|hi<<16 into the same W register
static std::vector<size_t> hashScan(const uint32_t* c, size_t n, uint32_t lo, uint32_t hi) {
    std::vector<size_t> r;
    for (size_t i = 0; i < n; ++i) {
        uint32_t a = c[i];
        if ((a & 0xFFE00000u) != 0x52800000u || ((a >> 5) & 0xFFFFu) != lo) continue;   // movz wN,#lo
        uint32_t rd = a & 31u;
        for (size_t j = i + 1; j < n && j <= i + 12; ++j) {
            uint32_t b = c[j];
            if ((b & 0xFFE0001Fu) == (0x72A00000u | rd) && ((b >> 5) & 0xFFFFu) == hi) { r.push_back(i); break; } // movk wN,#hi,lsl16
        }
    }
    return r;
}

// walk back to a likely prologue: "sub sp,sp,#imm" or "stp x29,x30,[sp,#-N]!"
static size_t funcStartGuess(const uint32_t* c, size_t i) {
    size_t k = i;
    for (size_t lim = 0; lim < 0x800 && k > 0; --k, ++lim) {
        uint32_t a = c[k];
        if ((a & 0xFF8003FFu) == 0xD10003FFu) return k;
        if ((a & 0xFFC07FFFu) == (0xA9800000u | (30u << 10) | (31u << 5) | 29u)) return k;
    }
    return i;
}

// PC-relative instructions change between builds, so signatures wildcard them
static bool pcRel(uint32_t i) {
    return (i & 0x9F000000u) == 0x90000000u ||  // adrp
           (i & 0x9F000000u) == 0x10000000u ||  // adr
           (i & 0x7C000000u) == 0x14000000u ||  // b / bl
           (i & 0xFF000010u) == 0x54000000u ||  // b.cond
           (i & 0x7E000000u) == 0x34000000u ||  // cbz / cbnz
           (i & 0x7E000000u) == 0x36000000u ||  // tbz / tbnz
           (i & 0x3B000000u) == 0x18000000u;    // ldr literal
}

static std::vector<int> sigPattern(const uint32_t* c, size_t n) {
    std::vector<int> p;
    for (size_t i = 0; i < n; ++i) {
        const bool w = pcRel(c[i]);
        for (int b = 0; b < 4; ++b) p.push_back(w ? -1 : static_cast<int>((c[i] >> (8 * b)) & 0xFF));
    }
    return p;
}

static std::string patStr(const std::vector<int>& p) {
    std::string s;
    for (size_t i = 0; i < p.size(); ++i) s += (i ? " " : "") + (p[i] < 0 ? std::string("??") : fmt("%02X", p[i]));
    return s;
}

static std::string sigFor(const Lib& L, uintptr_t addr) {
    if (!inExec(L, addr) || !inExec(L, addr + 160)) return "(address not in executable memory)";
    for (size_t n = 8; n <= 40; n += 4) {
        auto pat = sigPattern(reinterpret_cast<const uint32_t*>(addr), n);
        auto hits = scan(L, pat, true, false, 2);
        if (hits.size() == 1 && hits[0] == addr) return fmt("unique, %zu instructions: ", n) + patStr(pat);
    }
    return "(no unique signature up to 40 instructions)";
}

// ---- commands ----------------------------------------------------------------------------
static std::vector<std::string> args(const std::string& line, std::string& cmd) {
    std::vector<std::string> a;
    size_t p = line.find('<');
    cmd = line.substr(0, p);
    while (p != std::string::npos) {
        size_t q = line.find('>', p);
        if (q == std::string::npos) break;
        a.push_back(line.substr(p + 1, q - p - 1));
        p = line.find('<', q);
    }
    return a;
}

static std::vector<int> parseHexPattern(const std::string& s) {
    std::vector<int> p;
    std::istringstream is(s);
    std::string t;
    while (is >> t) p.push_back(t == "??" || t == "?" ? -1 : static_cast<int>(strtol(t.c_str(), nullptr, 16) & 0xFF));
    return p;
}

static std::string runCmd(const Lib& L, const std::string& line) {
    std::string cmd;
    auto a = args(line, cmd);
    std::string o;
    if (a.empty()) return "  (needs <argument>)\n";
    if (cmd == "findclass") {
        auto names = findNames(L, a[0], 80);
        o += fmt("  %zu RTTI name(s)\n", names.size());
        for (auto& n : names) o += fmt("  rva=0x%llx  %s\n    raw=%s\n", (unsigned long long)(n.addr - L.bias), demangleType(n.raw).c_str(), n.raw.c_str());
    } else if (cmd == "vt") {
        int slots = a.size() > 1 ? atoi(a[1].c_str()) : 16;
        auto names = findNames(L, a[0], 10);
        for (auto& n : names) { o += fmt("  %s\n", demangleType(n.raw).c_str()); dumpVtables(o, L, n, slots); }
        if (names.empty()) o += "  no RTTI name found\n";
    } else if (cmd == "hash") {
        uint32_t v = static_cast<uint32_t>(strtoul(a[0].c_str(), nullptr, 16));
        size_t total = 0;
        for (auto& g : L.segs) {
            if (!g.x || !g.r) continue;
            const auto* c = reinterpret_cast<const uint32_t*>(g.s);
            const size_t n = (g.e - g.s) / 4;
            for (size_t i : hashScan(c, n, v & 0xFFFFu, v >> 16)) {
                size_t st = funcStartGuess(c, i);
                o += fmt("  use @ rva=0x%llx   (function start guess rva=0x%llx)\n", (unsigned long long)(g.s + i * 4 - L.bias), (unsigned long long)(g.s + st * 4 - L.bias));
                if (++total >= 100) break;
            }
            if (total >= 100) break;
        }
        o += fmt("  %zu match(es)\n", total);
    } else if (cmd == "sig") {
        uintptr_t addr = L.bias + static_cast<uintptr_t>(strtoull(a[0].c_str(), nullptr, 16));
        if (inExec(L, addr)) {
            const auto* c = reinterpret_cast<const uint32_t*>(addr);
            size_t back = 0;
            for (size_t k = 0; k < 0x200 && inExec(L, addr - 4 * (k + 1)); ++k) {
                uint32_t x = *(c - (k + 1));
                if ((x & 0xFF8003FFu) == 0xD10003FFu) { back = k + 1; break; }
                if ((x & 0xFFC07FFFu) == (0xA9800000u | (30u << 10) | (31u << 5) | 29u)) { back = k + 1; break; }
                if (x == 0xD65F03C0u) break;   // ret: previous function ended
            }
            o += "  given   : " + sigFor(L, addr) + "\n";
            if (back) o += fmt("  note: a prologue-like instruction sits %zu insn(s) earlier, real start may be rva=0x%llx\n", back, (unsigned long long)(addr - 4 * back - L.bias)) +
                           "  earlier : " + sigFor(L, addr - 4 * back) + "\n";
        } else o += "  address not in executable memory\n";
    } else if (cmd == "scan") {
        auto hits = scan(L, parseHexPattern(a[0]), true, false, 60);
        for (auto h : hits) o += fmt("  rva=0x%llx\n", (unsigned long long)(h - L.bias));
        o += fmt("  %zu hit(s)\n", hits.size());
    } else o += "  unknown command (findclass, vt, hash, sig, scan)\n";
    return o;
}

// ---- file I/O + worker -------------------------------------------------------------------
static const char* DIR = "/storage/emulated/0/Android/media/org.levimc.launcher/examiner/";

static void writeFile(const std::string& path, const std::string& s) { std::ofstream f(path, std::ios::binary | std::ios::trunc); f << s; }
static std::string readFile(const std::string& path) { std::ifstream f(path, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }

static std::string trim(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\r' || s.back() == '\t' || s.back() == '\n')) s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(i);
}

static void process(const Lib& L, int run, const std::string& inPath, const std::string& outPath) {
    std::vector<std::string> lines;
    std::istringstream is(readFile(inPath));
    for (std::string l; std::getline(is, l);) { l = trim(l); if (!l.empty() && l[0] != '#') lines.push_back(l); }
    std::string o = fmt("=== Examiner run #%d | %zu command(s) | lib bias=0x%llx | addresses are RVAs ===\n", run, lines.size(), (unsigned long long)L.bias);
    writeFile(outPath, o + "[running...]\n");
    auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < lines.size(); ++i) {
        auto t1 = std::chrono::steady_clock::now();
        o += fmt("\n[%zu/%zu] %s\n", i + 1, lines.size(), lines[i].c_str());
        o += runCmd(L, lines[i]);
        long ms = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t1).count();
        o += fmt("  (%ld ms)\n", ms);
        writeFile(outPath, o + (i + 1 < lines.size() ? "[running...]\n" : ""));   // progress visible while it works
    }
    long total = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    writeFile(outPath, o + fmt("\n=== DONE in %ld ms ===\n", total));
}

static const char* DEFAULT_INPUT =
    "# one command per line, edit and save while the game runs; results appear in out.txt\n"
    "# findclass<text>        find RTTI class names containing text\n"
    "# vt<text>               vtables of matching classes (optional second arg: <vt><text><maxSlots>)\n"
    "# hash<0xC02D500F>       code that loads this 32-bit component id\n"
    "# sig<0xRVA>             byte signature for the function at RVA\n"
    "# scan<AA BB ?? CC>      byte pattern scan of executable code\n"
    "findclass<FallDamage>\n";

static void worker() {
    mkdir(DIR, 0777);
    const std::string dir = DIR, inPath = dir + "input.txt", outPath = dir + "out.txt";
    Lib L;
    for (int i = 0; i < 600; ++i) { L = findLib("libminecraftpe.so"); if (L.ok) break; usleep(500000); }
    if (!L.ok) { writeFile(outPath, "libminecraftpe.so not found in this process\n"); return; }
    struct stat st{};
    if (stat(inPath.c_str(), &st) != 0) writeFile(inPath, DEFAULT_INPUT);
    long long last = -1;
    int run = 0;
    for (;;) {
        if (stat(inPath.c_str(), &st) == 0) {
            long long key = static_cast<long long>(st.st_mtime) * 1000003LL + st.st_size;
            if (key != last) { last = key; process(L, ++run, inPath, outPath); }
        }
        usleep(1000000);
    }
}

}  // namespace ex

#ifndef EX_TEST
__attribute__((constructor)) static void examiner_init() { std::thread(ex::worker).detach(); }
#endif
