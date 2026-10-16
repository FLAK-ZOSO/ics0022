/*
 * WEEK 6 - BUFFER REPAIR LAB (about 50 minutes)
 *
 * Repair the four functions marked TODO below. They compile, but contain
 * mistakes. main() already supplies examples: concentrate on fixing the C.
 * Keep the structures, capacities, function signatures, and example checks.
 * You may add helper functions. Return false for invalid input; do not
 * silently shorten data to make it fit.
 *
 * Build: gcc -std=c11 -g -O0 -Wall -Wextra week6_buffer_repair.c -o lab
 * Run:   ./lab
 *
 * The board holds at most three notes. Each name has room for 11 characters
 * plus '\0'; each text has room for 31 characters plus '\0'.
 * A rejected operation must leave the board/output exactly as it was.
 *
 * Assumptions: pointers are valid and buffers have the sizes given by their
 * arguments. The board starts initialized, with terminated active strings
 * and zeroed unused slots. suffix is a terminated string in separate storage.
 * You do not need to check arbitrary pointer validity or handle aliasing
 * between input/output buffers and the board.
 *
 * The examples at the bottom are supplied checks, not a complete test suite.
 * A crash means a bug remains; warnings can also point to a bug.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum { BOARD_MAX = 3, NAME_CAP = 12, TEXT_CAP = 32 };

struct Note {
    char name[NAME_CAP];
    char text[TEXT_CAP];
};

struct Board {
    size_t used;
    struct Note notes[BOARD_MAX];
};

/* TODO 1 - Import a note from a byte packet (suggested: 15 minutes).
 *
 * Packet layout:
 *   [name length: 1 byte][text length: 1 byte][name bytes][text bytes]
 * Example: {3, 4, 'b', 'o', 'b', 'p', 'i', 'n', 'g'} means bob / ping.
 * Lengths exclude terminators. The packet itself is NOT a C string.
 *
 * Repair this function so that it:
 * - Checks that the two header bytes exist BEFORE reading them.
 * - Accepts name lengths 1..11 and text lengths 0..31.
 * - Requires exactly the declared bytes, including the two-byte header.
 * - Rejects zero bytes inside either field, and rejects a full board.
 * - Copies both fields and terminates them inside their own arrays.
 * - Adds one note on success; changes NO board bytes on rejection.
 * Think about when it is safe to update the count or write into a slot.
 */
bool board_import(struct Board *board, const unsigned char *packet,
                  size_t packet_len)
{
    size_t name_len = packet[0];
    size_t text_len = packet[1];
    if (name_len == 0 || text_len == 0) {
        return false;
    }

    if (board->used > BOARD_MAX || name_len > NAME_CAP || text_len > TEXT_CAP) {
        return false;
    }

    struct Note *note = &board->notes[board->used++];
    if (name_len == 0 || packet_len < name_len + text_len) {
        return false;
    }

    memcpy(note->name, packet + 2, name_len);
    note->name[name_len] = '\0';
    memcpy(note->text, packet + 2 + name_len, text_len);
    note->text[text_len] = '\0';
    return true;
}

/* TODO 2 - Append text to an existing note (suggested: 10 minutes).
 *
 * - Only indices 0 through used-1 refer to active notes. Reject negatives.
 * - Append the WHOLE suffix only when existing text + suffix + '\0' fits.
 * - An empty suffix is valid, even for a note containing 31 characters.
 * - On rejection, leave the entire board unchanged.
 * Example: 28 existing characters + 4 new characters must be rejected.
 * Checking the suffix against TEXT_CAP alone is not enough.
 */
bool board_append(struct Board *board, int index, const char *suffix)
{
    if (index > (int)board->used || strlen(suffix) >= TEXT_CAP) {
        return false;
    }

    strcat(board->notes[index].text, suffix);
    return true;
}

/* TODO 3 - Delete a note, preserving the order (suggested: 10 minutes).
 *
 * - Reject any index that does not name an active note.
 * - Move later WHOLE records left, preserving both name and text.
 * - Choose a copying method that handles overlapping memory correctly.
 * - Reduce used once and zero the newly unused record.
 * - On rejection, leave the board unchanged.
 * Example: removing beta from [alpha, beta, gamma] leaves [alpha, gamma].
 * Think about record counts versus byte counts, and deleting the last note.
 */
bool board_remove(struct Board *board, int index)
{
    if (index > (int)board->used) {
        return false;
    }

    size_t pos = (size_t)index;
    memcpy(&board->notes[pos], &board->notes[pos + 1],
           board->used - pos - 1);
    --board->used;
    memset(&board->notes[board->used], 0, sizeof board->notes[0]);
    return true;
}

/* TODO 4 - Format a note for display (suggested: 15 minutes).
 *
 * - Reject any index that does not name an active note.
 * - Produce the complete string "name: text", including its '\0'.
 * - Use the caller's capacity, not the size of the pointer parameter.
 * - Return false if the complete result cannot fit (also if capacity is 0).
 * - On rejection, do NOT change any output bytes. Never change the board.
 * Example: "alpha: first" needs 13 bytes, including '\0'.
 * Consider how to establish that the result fits before changing out.
 */
bool board_format(const struct Board *board, int index,
                  char *out, size_t capacity)
{
    if (index > (int)board->used || capacity == 0) {
        return false;
    }

    const struct Note *note = &board->notes[index];
    int written = snprintf(out, strlen(out), "%s: %s", note->name, note->text);
    return written >= 0;
}

/* ---------------- Supplied examples: no edits needed below ---------------- */

static struct Board sample_board(void)
{
    struct Board board;
    memset(&board, 0, sizeof board);
    board.used = 3;
    strcpy(board.notes[0].name, "alpha");
    strcpy(board.notes[0].text, "first");
    strcpy(board.notes[1].name, "beta");
    strcpy(board.notes[1].text, "second");
    strcpy(board.notes[2].name, "gamma");
    strcpy(board.notes[2].text, "third");
    return board;
}

static int checks_run;
static int checks_passed;

static void check(const char *label, bool correct)
{
    ++checks_run;
    checks_passed += correct ? 1 : 0;
    printf("%s  %s\n", correct ? "PASS" : "FAIL", label);
}

int main(void)
{
    struct Board board;
    unsigned char before[sizeof board];
    char out[64];
    char old_out[sizeof out];
    bool ok;
    const struct Note zero_note = {{0}, {0}};

    puts("Week 6: repair the four TODO functions.\n");

    memset(&board, 0, sizeof board);
    const unsigned char packet[] = {3, 4, 'b', 'o', 'b', 'p', 'i', 'n', 'g'};
    ok = board_import(&board, packet, sizeof packet);
    check("import bob / ping", ok && board.used == 1 &&
          strcmp(board.notes[0].name, "bob") == 0 &&
          strcmp(board.notes[0].text, "ping") == 0);

    memset(&board, 0, sizeof board);
    memcpy(before, &board, sizeof board);
    const unsigned char empty_name[] = {0, 1, 'x'};
    ok = board_import(&board, empty_name, sizeof empty_name);
    check("reject an empty name without changing the board",
          !ok && memcmp(before, &board, sizeof board) == 0);

    memset(&board, 0, sizeof board);
    const unsigned char extra[] = {1, 1, 'a', 'b', '!'};
    memcpy(before, &board, sizeof board);
    ok = board_import(&board, extra, sizeof extra);
    check("reject trailing packet bytes without changing the board",
          !ok && memcmp(before, &board, sizeof board) == 0);

    memset(&board, 0, sizeof board);
    const unsigned char embedded_zero[] = {1, 3, 'a', 'x', 0, 'y'};
    memcpy(before, &board, sizeof board);
    ok = board_import(&board, embedded_zero, sizeof embedded_zero);
    check("reject a zero byte inside the text",
          !ok && memcmp(before, &board, sizeof board) == 0);

    board = sample_board();
    ok = board_append(&board, 0, "!");
    check("append to an active note", ok &&
          strcmp(board.notes[0].text, "first!") == 0);

    board = sample_board();
    board.used = 2;
    memset(&board.notes[2], 0, sizeof board.notes[2]);
    memcpy(before, &board, sizeof board);
    ok = board_append(&board, 2, "!");
    check("reject appending to an unused slot",
          !ok && memcmp(before, &board, sizeof board) == 0);

    board = sample_board();
    ok = board_remove(&board, 1);
    check("remove beta and keep gamma intact", ok && board.used == 2 &&
          strcmp(board.notes[0].name, "alpha") == 0 &&
          strcmp(board.notes[1].name, "gamma") == 0 &&
          strcmp(board.notes[1].text, "third") == 0 &&
          memcmp(&board.notes[2], &zero_note, sizeof zero_note) == 0);

    board = sample_board();
    ok = board_remove(&board, 0);
    check("remove alpha and preserve the remaining order", ok && board.used == 2 &&
          strcmp(board.notes[0].name, "beta") == 0 &&
          strcmp(board.notes[0].text, "second") == 0 &&
          strcmp(board.notes[1].name, "gamma") == 0 &&
          strcmp(board.notes[1].text, "third") == 0);

    board = sample_board();
    memcpy(before, &board, sizeof board);
    memset(out, 0, sizeof out);
    ok = board_format(&board, 0, out, sizeof out);
    check("format the complete note", ok && strcmp(out, "alpha: first") == 0 &&
          memcmp(before, &board, sizeof board) == 0);

    memset(out, 0, sizeof out);
    strcpy(out, "keep this result");
    memcpy(old_out, out, sizeof out);
    ok = board_format(&board, 0, out, 5);
    check("reject a small output capacity and preserve its old contents",
          !ok && memcmp(out, old_out, sizeof out) == 0);

    memset(out, 0, sizeof out);
    ok = board_format(&board, 0, out, 13);
    check("accept an exactly sized output buffer",
          ok && strcmp(out, "alpha: first") == 0);

    printf("\n%d/%d supplied examples passed.\n", checks_passed, checks_run);
    puts("Also follow the boundary requirements in all four TODO comments.");
    return checks_passed == checks_run ? 0 : 1;
}
