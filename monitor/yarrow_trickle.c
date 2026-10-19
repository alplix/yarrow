#include "yarrow_trickle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ypending_add(ypending_t *p, uint32_t start, uint32_t count)
{
    int dropped = 0;
    if (p->n == YT_MAX_PENDING) {
        memmove(p->start, p->start + 1, (YT_MAX_PENDING - 1) * sizeof p->start[0]);
        memmove(p->count, p->count + 1, (YT_MAX_PENDING - 1) * sizeof p->count[0]);
        p->n--;
        dropped = -1;
    }
    p->start[p->n] = start;
    p->count[p->n] = count;
    p->n++;
    return dropped;
}

void ypending_drop(ypending_t *p, uint32_t k)
{
    if (k >= p->n) {
        p->n = 0;
        return;
    }
    memmove(p->start, p->start + k, (p->n - k) * sizeof p->start[0]);
    memmove(p->count, p->count + k, (p->n - k) * sizeof p->count[0]);
    p->n -= k;
}

uint32_t ypending_xml(const ypending_t *p, const ydev_t *d, char *out, size_t cap)
{
    if (!p->n)
        return 0;
    uint32_t k = 1;
    while (k < p->n && (p->start[k] - p->start[0]) / d->interval < YT_MAX_SPAN)
        k++;
    int n = snprintf(out, cap, "<yarrow v=\"1\" start=\"%u\" interval=\"%u\" dev=\"%s\" fw=\"%s\" sens=\"%u\" dead=\"%u\" cc=\"%s\" city=\"%s\" bins=\"",
                     (unsigned)p->start[0], (unsigned)d->interval, d->uid, d->fw, (unsigned)d->sens_x100,
                     (unsigned)d->dead_us, d->cc, d->city);
    if (n < 0 || (size_t)n >= cap)
        return 0;
    uint32_t slots = (p->start[k - 1] - p->start[0]) / d->interval + 1, at = 0;
    for (uint32_t s = 0; s < slots; s++) {
        uint32_t want = p->start[0] + s * d->interval;
        int m;
        if (at < k && p->start[at] == want)
            m = snprintf(out + n, cap - (size_t)n, "%s%u", s ? "," : "", (unsigned)p->count[at++]);
        else
            m = snprintf(out + n, cap - (size_t)n, "%s-", s ? "," : "");
        if (m < 0 || (size_t)(n + m) >= cap)
            return 0;
        n += m;
    }
    if (snprintf(out + n, cap - (size_t)n, "\"/>") >= (int)(cap - (size_t)n))
        return 0;
    return k;
}

size_t ypending_save(const ypending_t *p, char *out, size_t cap)
{
    size_t n = 0;
    for (uint32_t i = 0; i < p->n; i++) {
        int m = snprintf(out + n, cap - n, "%u %u\n", (unsigned)p->start[i], (unsigned)p->count[i]);
        if (m < 0 || n + (size_t)m >= cap)
            break;
        n += (size_t)m;
    }
    return n;
}

void ypending_load(ypending_t *p, const char *text)
{
    p->n = 0;
    unsigned s, c;
    int used;
    if (text[0] == 'D') {                      /* identity line written by the monitor */
        const char *nl = strchr(text, 10);
        if (!nl)
            return;
        text = nl + 1;
    }
    while (sscanf(text, "%u %u%n", &s, &c, &used) == 2) {
        ypending_add(p, s, c);
        text += used;
    }
}

int ydev_valid_place(const char *cc, const char *city)
{
    if (strlen(cc) != 2 || cc[0] < 'A' || cc[0] > 'Z' || cc[1] < 'A' || cc[1] > 'Z')
        return 0;
    size_t n = strlen(city);
    if (n < 2 || n > 40)
        return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)city[i];
        if (c == '<' || c == '>' || c == '"' || c == '&' || c < 32 || c == 127)
            return 0;
        if ((c >= '0' && c <= '9') || c == '_' || c == '=' || c == ';' || c == '\\' || c == '/')
            return 0;
    }
    return 1;
}
