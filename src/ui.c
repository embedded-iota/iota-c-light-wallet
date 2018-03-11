#include "ui.h"
#include <string.h>
#include "common.h"
#include "aux.h"

char half_top[21];
char top_str[21];
char mid_str[21];
char bot_str[21];
char half_bot[21];

// flags for turning on/off certain glyphs
char glyph_bar_l[2], glyph_bar_r[2];
char glyph_bar_l_c[2], glyph_bar_r_c[2];
char glyph_cross[2], glyph_check[2];
char glyph_up[2], glyph_down[2];
char glyph_warn[2];

uint8_t ui_state;
uint8_t text_idx;

// tx information
int64_t bal = 0;
int64_t pay = 0;
char addr[21];

// matrix holds layout of state transitions
static uint8_t state_transitions[TOTAL_STATES][3];

// ----------- local function prototypes
void init_state_transitions(void);
void ui_display_state(void);
void ui_handle_button(uint8_t button_mask);
void ui_handle_text_array(uint8_t translated_mask);
void ui_transition_state(unsigned int button_mask);

unsigned int bagl_ui_nanos_screen_button(unsigned int, unsigned int);

// *************************
// Ledger Nano S specific UI
// *************************
// one dynamic screen that changes based on the ui state

// clang-format off
static const bagl_element_t bagl_ui_nanos_screen[] = {
    // {type, userid, x, y, width, height, stroke, radius, fill,
    // fgcolor, bgcolor, fontid, iconid}, text .....
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
        0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},
    
    {{BAGL_LABELINE, 0x01, 0, 3, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
        BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        half_top, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
        BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        top_str, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x01, 0, 18, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
        BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        mid_str, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x01, 0, 24, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
        BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        bot_str, 0, 0, 0, NULL, NULL, NULL},
    
    {{BAGL_LABELINE, 0x01, 0, 33, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
        BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        half_bot, 0, 0, 0, NULL, NULL, NULL},
    
    {{BAGL_ICON, 0x00, 3, -3, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
        BAGL_GLYPH_ICON_LESS}, glyph_bar_l, 0, 0, 0, NULL, NULL, NULL},
    
    {{BAGL_ICON, 0x00, 117, -3, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
        BAGL_GLYPH_ICON_LESS}, glyph_bar_r, 0, 0, 0, NULL, NULL, NULL},
    
    {{BAGL_ICON, 0x00, 3, 0, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
        BAGL_GLYPH_ICON_LESS}, glyph_bar_l_c, 0, 0, 0, NULL, NULL, NULL},
    
    {{BAGL_ICON, 0x00, 117, 0, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
        BAGL_GLYPH_ICON_LESS}, glyph_bar_r_c, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0x000000, 0x000000, 0,
        BAGL_GLYPH_ICON_CROSS}, glyph_cross, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
        BAGL_GLYPH_ICON_CHECK}, glyph_check, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0x000000, 0x000000, 0,
        BAGL_GLYPH_ICON_UP}, glyph_up, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
        BAGL_GLYPH_ICON_DOWN}, glyph_down, 0, 0, 0, NULL, NULL, NULL},
    
    {{BAGL_ICON, 0x00, 9, 11, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
        BAGL_GLYPH_ICON_WARNING_BADGE}, glyph_warn, 0, 0, 0, NULL, NULL, NULL}};
// clang-format on

/* ------------------- DISPLAY UI FUNCTIONS -------------
 ---------------------------------------------------------
 --------------------------------------------------------- */
void ui_init(bool first_run)
{
    init_state_transitions();
    text_idx = 0;

    if (first_run)
        ui_state = STATE_INIT;
    else
        ui_state = STATE_WELCOME;

    ui_display_state();
}

void ui_reset()
{
    bal = 0;
    pay = 0;
    memset(addr, '\0', sizeof(addr));

    ui_state = STATE_WELCOME;
    ui_display_state();
}

void ui_record_addr(const char *a, uint8_t len)
{
    // length 81 or 82 means full address
    if (len == 81 || len == 82) {
        // Convert into abbreviated seeed
        memcpy(addr, a, 6);          // first 6 chars of address
        memcpy(addr + 6, "...", 3);  // copy ...
        memcpy(addr + 9, a + 75, 7); // copy last 6 chars + null
    }
    else if (len <= 21) {
        memcpy(addr, a, len);
    }
}

void ui_display_welcome()
{
    ui_state = STATE_WELCOME;
    ui_display_state();
}

void ui_display_calc()
{
    ui_state = STATE_CALC;
    ui_display_state();
}

void ui_display_recv()
{
    ui_state = STATE_RECV;
    ui_display_state();
}

void ui_sign_tx(int64_t b, int64_t p, const char *a, uint8_t len)
{
    bal = b;
    pay = p;
    ui_record_addr(a, len);

    ui_state = STATE_TX_SIGN;

    ui_display_state();
}

// write_display(&words, sizeof(words), TYPE_STR);
// write_display(&int_val, sizeof(int_val), TYPE_INT);
void write_display(void *o, uint8_t sz, uint8_t t, uint8_t p)
{
    // don't allow messages greater than 20
    if (sz > 21)
        sz = 21;

    char *c_ptr = NULL;

    switch (p) {
    case TOP_H:
        c_ptr = half_top;
        break;
    case TOP:
        c_ptr = top_str;
        break;
    case BOT:
        c_ptr = bot_str;
        break;
    case BOT_H:
        c_ptr = half_bot;
        break;
    case MID:
    default:
        c_ptr = mid_str;
        break;
    }

    // NULL value sets line blank
    if (o == NULL) {
        c_ptr[0] = '\0';
        return;
    }

    // uint32_t/int(half this) [0, 4,294,967,295] -
    // ledger does not support long - USE %d not %i!
    if (t == TYPE_INT) {
        snprintf(c_ptr, sz, "%d", *(int32_t *)o);
    }
    else if (t == TYPE_UINT) {
        snprintf(c_ptr, sz, "%u", *(uint32_t *)o);
    }
    else if (t == TYPE_STR) {
        snprintf(c_ptr, sz, "%s", (char *)o);
    }
}

// ui_display_message(top, szof(top), TYPE_TOP,
//                  mid, szof(mid), TYPE_MID,
//                  bot, szof(bot) TYPE_BOT);
void ui_display_message(void *o, uint8_t sz, uint8_t t, void *o2, uint8_t sz2,
                        uint8_t t2, void *o3, uint8_t sz3, uint8_t t3)
{
    write_display(o, sz, t, TOP);
    write_display(o2, sz2, t2, MID);
    write_display(o3, sz3, t3, BOT);

    UX_DISPLAY(bagl_ui_nanos_screen, NULL);
}

void clear_display()
{
    write_display(NULL, 21, TYPE_STR, TOP_H);
    write_display(NULL, 21, TYPE_STR, TOP);
    write_display(NULL, 21, TYPE_STR, MID);
    write_display(NULL, 21, TYPE_STR, BOT);
    write_display(NULL, 21, TYPE_STR, BOT_H);
}


/* -------------------- SCREEN BUTTON FUNCTIONS ---------------
 ---------------------------------------------------------------
 --------------------------------------------------------------- */
unsigned int bagl_ui_nanos_screen_button(unsigned int button_mask,
                                         unsigned int button_mask_counter)
{
    ui_transition_state(button_mask);

    return 0;
}


const bagl_element_t *io_seproxyhal_touch_exit(const bagl_element_t *e)
{
    // Go back to the dashboard
    os_sched_exit(0);
    return NULL;
}

const bagl_element_t *io_seproxyhal_touch_deny(const bagl_element_t *e)
{
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);

    return 0; // do not redraw the widget
}

const bagl_element_t *io_seproxyhal_touch_approve(const bagl_element_t *e)
{
    unsigned int tx = 0;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

    return 0; // do not redraw the widget
}


/* --------- STATE RELATED FUNCTIONS ----------- */

// Turns glyphs on or off
void display_on(char *c)
{
    if (c != NULL)
        c[0] = '\0';
}

void display_off(char *c)
{
    if (c != NULL) {
        c[0] = '.';
        c[1] = '\0';
    }
}

// turns on only 2 glyphs
void display_glyphs(char *c1, char *c2)
{
    // turn off all glyphs
    display_off(glyph_bar_l);
    display_off(glyph_bar_r);
    display_off(glyph_bar_l_c);
    display_off(glyph_bar_r_c);
    display_off(glyph_cross);
    display_off(glyph_check);
    display_off(glyph_up);
    display_off(glyph_down);
    display_off(glyph_warn);

    // turn on ones we want
    display_on(c1);
    display_on(c2);
}

// combine glyphs with bars along top for confirm
void display_glyphs_confirm(char *c1, char *c2)
{
    // turn off all glyphs
    display_off(glyph_cross);
    display_off(glyph_check);
    display_off(glyph_up);
    display_off(glyph_down);
    display_off(glyph_warn);

    // turn on ones we want
    display_on(glyph_bar_l);
    display_on(glyph_bar_r);
    display_on(glyph_bar_l_c);
    display_on(glyph_bar_r_c);
    display_on(c1);
    display_on(c2);
}

// combine glyphs with bars along top for exit
void display_glyphs_exit(char *c1, char *c2)
{
    // turn off all glyphs
    display_off(glyph_cross);
    display_off(glyph_check);
    display_off(glyph_up);
    display_off(glyph_down);
    display_off(glyph_warn);
    display_off(glyph_bar_l_c);
    display_off(glyph_bar_r_c);

    // turn on ones we want
    display_on(glyph_bar_l);
    display_on(glyph_bar_r);
    display_on(c1);
    display_on(c2);
}

uint8_t ui_translate_mask(unsigned int button_mask)
{
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        return BUTTON_L;
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        return BUTTON_R;
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT | BUTTON_LEFT:
        return BUTTON_B;
    default:
        return BUTTON_BAD;
    }
}

bool state_is(uint8_t state)
{
    return ui_state == state;
}

void ui_transition_state(unsigned int button_mask)
{
    uint8_t translated_mask = ui_translate_mask(button_mask);

    // make sure we only transition on valid button presses
    if (translated_mask == BUTTON_BAD)
        return;

    ui_handle_button(translated_mask);

    ui_state = state_transitions[ui_state][translated_mask];

    // special handles text array screens
    ui_handle_text_array(translated_mask);

    if (state_is(STATE_EXIT))
        io_seproxyhal_touch_exit(NULL);

    // after transitioning, immediately display new state
    ui_display_state();
}

void write_text_array(char *array, uint8_t len)
{
    clear_display();

    if (text_idx > 0)
        write_display(array + (21 * (text_idx - 1)), 21, TYPE_STR, TOP_H);

    write_display(array + (21 * text_idx), 21, TYPE_STR, MID);

    if (text_idx < len - 1)
        write_display(array + (21 * (text_idx + 1)), 21, TYPE_STR, BOT_H);
}

void get_init_text_array(char *msg)
{
    memset(msg, '\0', INIT_ARR_LEN * 21);

    memcpy(msg, "WARNING!", 9);
    memcpy(msg + 21, "IOTA is not like", 17);
    memcpy(msg + 42, "other cryptos!", 15);
    memcpy(msg + 63, "Visit iota.org/nanos", 21);
    memcpy(msg + 84, "to learn more", 14);
}

void ui_display_state()
{
    switch (ui_state) {
        /* ------------ INIT *TEXT_ARRAY* -------------- */
    case STATE_INIT: {
        char msg[INIT_ARR_LEN * 21];
        get_init_text_array(msg);
        write_text_array(msg, INIT_ARR_LEN);

        if (text_idx == 0)
            display_glyphs(glyph_warn, glyph_down);
        else if (text_idx == INIT_ARR_LEN - 1)
            display_glyphs_exit(glyph_up, glyph_check);
        else
            display_glyphs(glyph_up, glyph_down);
    } break;
        /* ------------ WELCOME -------------- */
    case STATE_WELCOME: {
        clear_display();
        write_display("Welcome to IOTA", 21, TYPE_STR, MID);

        display_glyphs_exit(NULL, NULL);
    } break;
        /* ------------ TX BAL -------------- */
    case STATE_TX_BAL: {
        clear_display();
        write_display("Total Balance:", 21, TYPE_STR, TOP);
        write_display(&bal, 21, TYPE_UINT, BOT);

        display_glyphs_exit(glyph_up, glyph_down);
    } break;
        /* ------------ TX SPEND -------------- */
    case STATE_TX_SPEND: {
        clear_display();
        write_display("Send Amt:", 21, TYPE_STR, TOP);
        write_display(&pay, 21, TYPE_UINT, BOT);

        display_glyphs_exit(glyph_up, glyph_down);
    } break;
        /* ------------ TX ADDR -------------- */
    case STATE_TX_ADDR: {
        clear_display();
        write_display("Dest Address:", 21, TYPE_STR, TOP);
        write_display(addr, 21, TYPE_STR, BOT);

        display_glyphs_confirm(glyph_up, glyph_down);
    } break;
        /* ------------ TX CALCULATING -------------- */
    case STATE_CALC: {
        clear_display();
        write_display("Calculating...", 21, TYPE_STR, MID);

        display_glyphs_exit(NULL, NULL);
    } break;
        /* ------------ TX RECEIVING -------------- */
    case STATE_RECV: {
        clear_display();
        write_display("Receiving tx...", 21, TYPE_STR, MID);

        display_glyphs_exit(NULL, NULL);
    } break;
        /* ------------ UNKNOWN STATE -------------- */
    default: {
        clear_display();
        write_display("UI ERROR", 21, TYPE_STR, MID);

        display_glyphs_exit(NULL, NULL);
    } break;
    }

    UX_DISPLAY(bagl_ui_nanos_screen, NULL);
}

void ui_handle_button(uint8_t button_mask)
{
    /* ------------- APPROVE TX --------------- */
    if (ui_state == STATE_TX_ADDR && button_mask == BUTTON_B)
        user_sign();
}

// moves text, handles special exit conditions
void ui_handle_text_array(uint8_t translated_mask)
{
    uint8_t array_sz = 0;

    switch (ui_state) {
        /* ------------ STATE INIT -------------- */
    case STATE_INIT:
        array_sz = INIT_ARR_LEN - 1;

        if (translated_mask == BUTTON_B && text_idx == array_sz) {
            ui_state = STATE_WELCOME;
            return;
        }
        break;
        /* ------------ DEFAULT -------------- */
    default:
        text_idx = 0;
        return;
    }

    // auto incr or decr text_idx
    if (translated_mask == BUTTON_L)
        text_idx = MAX(0, text_idx - 1);
    else if (translated_mask == BUTTON_R)
        text_idx = MIN(array_sz, text_idx + 1);
}

void init_state_transitions()
{
    /* ------------- WELCOME --------------- */
    state_transitions[STATE_WELCOME][BUTTON_L] = STATE_WELCOME;
    state_transitions[STATE_WELCOME][BUTTON_R] = STATE_WELCOME;
    state_transitions[STATE_WELCOME][BUTTON_B] = STATE_EXIT;
    /* ------------- TX BALANCE --------------- */
    state_transitions[STATE_TX_BAL][BUTTON_L] = STATE_TX_ADDR;
    state_transitions[STATE_TX_BAL][BUTTON_R] = STATE_TX_SPEND;
    state_transitions[STATE_TX_BAL][BUTTON_B] = STATE_EXIT;
    /* ------------- TX SPEND --------------- */
    state_transitions[STATE_TX_SPEND][BUTTON_L] = STATE_TX_BAL;
    state_transitions[STATE_TX_SPEND][BUTTON_R] = STATE_TX_ADDR;
    state_transitions[STATE_TX_SPEND][BUTTON_B] = STATE_EXIT;
    /* ------------- TX ADDR --------------- */
    state_transitions[STATE_TX_ADDR][BUTTON_L] = STATE_TX_SPEND;
    state_transitions[STATE_TX_ADDR][BUTTON_R] = STATE_TX_BAL;
    state_transitions[STATE_TX_ADDR][BUTTON_B] = STATE_CALC;
    /* ------------- TX CALCULATING --------------- */
    state_transitions[STATE_CALC][BUTTON_L] = STATE_CALC;
    state_transitions[STATE_CALC][BUTTON_R] = STATE_CALC;
    state_transitions[STATE_CALC][BUTTON_B] = STATE_EXIT;
    /* ------------- TX RECEIVING --------------- */
    state_transitions[STATE_RECV][BUTTON_L] = STATE_RECV;
    state_transitions[STATE_RECV][BUTTON_R] = STATE_RECV;
    state_transitions[STATE_RECV][BUTTON_B] = STATE_EXIT;
    /* ------------- INIT --------------- */
    state_transitions[STATE_INIT][BUTTON_L] = STATE_INIT;
    state_transitions[STATE_INIT][BUTTON_R] = STATE_INIT;
    state_transitions[STATE_INIT][BUTTON_B] = STATE_WELCOME;
}
