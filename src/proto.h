/* Serial command protocol (USB serial or UART). Plain text lines, one command per line, replies start with OK or ERR.
 *
 *   PING                          OK pong
 *   INFO                          OK yarrow=0.1 stage=.. verified=.. time=.. theme=.. profile=.. pages=.. pagesize=..
 *   TIME | TIME <unix>            read or set the clock
 *   LIVE 1 | LIVE 0               stream "L <unix> <nSv/h> <cpm> <E x10> <H x100> <battery %>" every second
 *   LOGINFO                       OK pages=.. pagesize=.. used=.. oldest=.. newest=..
 *   LOGPAGE <n>                   OK <n> <size>, base64 lines of the raw logical page, END
 *   EVENTS                        OK <count>, "E <start> <duration> <peak nSv/h> <level>" lines, END
 *   CAL | CAL <sens> <dead> <e> <h>   read or set the calibration
 *   SET THEME <n>                 choose a theme
 */
#ifndef YARROW_PROTO_H
#define YARROW_PROTO_H

#include "app.h"

#define PROTO_LINE 96

typedef struct {
    app_t *app;
    char line[PROTO_LINE];
    uint8_t len;
    bool overflow;
    bool live;
    uint32_t live_sent_s;
} proto_t;

void proto_init(proto_t *p, app_t *app);

/* Reads whatever the serial port has and answers complete commands. */
void proto_poll(proto_t *p);

/* Call once per second: sends the live line if streaming is on. */
void proto_tick(proto_t *p, uint32_t now_s);

/* Feed bytes directly (used by tests and by poll). */
void proto_feed(proto_t *p, const uint8_t *data, uint32_t n);

#endif
