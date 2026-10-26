// BOINC host for the monitor. Preferences come from the project preferences on the web site
// (<yarrow_interval>, <yarrow_country>, <yarrow_city>), so the volunteer sets them once for all computers.
#include "yarrow_glue.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "boinc_api.h"
#include "filesys.h"

static std::string tag(const std::string &xml, const char *name)
{
    std::string open = std::string("<") + name + ">", close = std::string("</") + name + ">";
    size_t a = xml.find(open);
    if (a == std::string::npos)
        return "";
    a += open.size();
    size_t b = xml.find(close, a);
    if (b == std::string::npos)
        return "";
    std::string v = xml.substr(a, b - a);
    while (!v.empty() && (v.back() == ' ' || v.back() == '\n' || v.back() == '\r'))
        v.pop_back();
    while (!v.empty() && (v.front() == ' ' || v.front() == '\n'))
        v.erase(0, 1);
    return v;
}

extern "C" {

int glue_init(int, char **, yprefs_t *p)
{
    BOINC_OPTIONS opts;
    boinc_options_defaults(opts);
    if (boinc_init_options(&opts) != 0)
        return -1;
    APP_INIT_DATA aid;
    boinc_get_init_data(aid);
    std::string prefs = aid.project_preferences ? aid.project_preferences : "";
    std::string iv = tag(prefs, "yarrow_interval");
    p->interval = iv.empty() ? 600 : (uint32_t)atoi(iv.c_str());
    snprintf(p->cc, sizeof p->cc, "%s", tag(prefs, "yarrow_country").c_str());
    snprintf(p->city, sizeof p->city, "%s", tag(prefs, "yarrow_city").c_str());
    return 0;
}

static std::string lock_file()
{
    APP_INIT_DATA aid;
    boinc_get_init_data(aid);
    static std::string p;
    p = std::string(aid.project_dir[0] ? aid.project_dir : ".") + "/yarrow_monitor.lock";
    return p;
}

static std::string state_file()
{
    std::string out;
    char resolved[512];
    if (boinc_resolve_filename("yarrow_state.txt", resolved, sizeof resolved) == 0)
        out = resolved;
    else
        out = "yarrow_state.txt";
    return out;
}

size_t glue_checkpoint_read(char *buf, size_t cap)
{
    FILE *f = boinc_fopen(state_file().c_str(), "rb");
    if (!f)
        return 0;
    size_t n = fread(buf, 1, cap, f);
    fclose(f);
    return n;
}

void glue_checkpoint_write(const char *text, size_t n)
{
    std::string tmp = state_file() + ".tmp";
    FILE *f = boinc_fopen(tmp.c_str(), "wb");
    if (!f)
        return;
    fwrite(text, 1, n, f);
    fclose(f);
    boinc_rename(tmp.c_str(), state_file().c_str());
    boinc_checkpoint_completed();
}

int glue_send_trickle(const char *text)
{
    return boinc_send_trickle_up(const_cast<char *>("yarrow"), const_cast<char *>(text));
}

const char *glue_lock_path(void)
{
    static std::string p;
    p = lock_file();
    return p.c_str();
}

int glue_quit_requested(void)
{
    BOINC_STATUS st;
    boinc_get_status(&st);
    return st.quit_request || st.abort_request;
}

void glue_log(const char *line) { fprintf(stderr, "%s\n", line); }

void glue_done(int status)
{
    char path[512];
    if (boinc_resolve_filename("out", path, sizeof path) == 0) {   // the result template wants one small output file
        FILE *f = boinc_fopen(path, "w");
        if (f) {
            fputs("yarrow monitor finished\n", f);
            fclose(f);
        }
    }
    boinc_finish(status);
}

void glue_wait_for_prefs(const char *why) { boinc_temporary_exit(3600, why, true); }

/* no detector for five minutes: ask again in half an hour. (It was six hours: a counter plugged in again after a pause then waited
 * most of a day for its task to wake up, and a task that is only waiting costs nothing.) */
void glue_no_detector(const char *why) { boinc_temporary_exit(1800, why, true); }
void glue_other_monitor(const char *why) { boinc_temporary_exit(2 * 3600, why, true); }

}
