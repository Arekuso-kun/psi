#include "input.h"
#include <string.h>

void si_input_init(SiInput *in) { memset(in, 0, sizeof(*in)); }
static void set_mask(SiInput *in, unsigned source, uint8_t mask) {
    uint8_t pressed = (uint8_t)(mask & ~in->mask[source]);
    in->edges |= pressed & (SI_PAUSE | SI_START);
    if (pressed & SI_LEFT) in->order[source][0] = ++in->sequence;
    if (pressed & SI_RIGHT) in->order[source][1] = ++in->sequence;
    in->mask[source] = mask;
}
void si_input_local(SiInput *in, uint8_t mask) { set_mask(in, 0, mask & 0x7f); }
void si_input_remote(SiInput *in, uint8_t mask, uint32_t now) {
    set_mask(in, 1, mask & 0x7f);
    in->remote_at = now;
    in->remote_valid = 1;
}
SiControls si_input_poll(SiInput *in, uint32_t now) {
    SiControls c = {0};
    uint8_t mask, volume;
    uint32_t left = 0, right = 0;
    unsigned i;
    if (in->remote_valid && now - in->remote_at >= 350) {
        set_mask(in, 1, 0);
        in->remote_valid = 0;
    }
    mask = in->mask[0] | in->mask[1];
    for (i = 0; i < 2; ++i) {
        if ((in->mask[i] & SI_LEFT) && in->order[i][0] > left) left = in->order[i][0];
        if ((in->mask[i] & SI_RIGHT) && in->order[i][1] > right) right = in->order[i][1];
    }
    c.move = (mask & SI_LEFT) ? -1 : 0;
    if ((mask & SI_RIGHT) && (!(mask & SI_LEFT) || right > left)) c.move = 1;
    c.fire = (mask & SI_FIRE) != 0;
    c.pause = (in->edges & SI_PAUSE) != 0;
    c.start = (in->edges & SI_START) != 0;
    in->edges = 0;
    volume = mask & (SI_VOL_UP | SI_VOL_DOWN);
    if (volume && (volume != in->previous_volume || now - in->volume_at >= 200)) {
        if (volume == SI_VOL_UP) c.volume_delta = 10;
        if (volume == SI_VOL_DOWN) c.volume_delta = -10;
        in->volume_at = now;
    }
    in->previous_volume = volume;
    return c;
}

SiCommand si_protocol_parse(const char *line) {
    SiCommand c = {SI_BAD_COMMAND, 0};
    unsigned n = 0, count = 0;
    const char *p;
    if (!line) return c;
    if (!strcmp(line, "PING")) { c.type = SI_PING_COMMAND; return c; }
    if (!strcmp(line, "STATE?")) { c.type = SI_STATE_COMMAND; return c; }
    if (line[0] == 'K' && line[1] == ',') {
        for (p = line + 2; *p; ++p) {
            unsigned v;
            if (*p >= '0' && *p <= '9') v = (unsigned)(*p - '0');
            else if (*p >= 'A' && *p <= 'F') v = (unsigned)(*p - 'A') + 10;
            else if (*p >= 'a' && *p <= 'f') v = (unsigned)(*p - 'a') + 10;
            else return c;
            if (++count > 2) return c;
            n = n * 16 + v;
        }
        if (!count || n > 0x7f) return c;
        c.type = SI_KEYS_COMMAND;
    } else if (!strncmp(line, "VOL,", 4)) {
        for (p = line + 4; *p; ++p) {
            if (*p < '0' || *p > '9' || ++count > 3) return c;
            n = n * 10 + (unsigned)(*p - '0');
        }
        if (!count || n > 100) return c;
        c.type = SI_VOLUME_COMMAND;
    }
    c.value = (uint8_t)n;
    return c;
}
