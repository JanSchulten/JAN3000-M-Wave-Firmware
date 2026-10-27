/* Review-only repros; firmware sources remain unmodified. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int review_automation_sound(void)
{
    ui_power_on(); frames(400);
    TSEL->p[P_ATK] = 23;
    TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60;
    motion_set_lock(TSEL, 0, P_ATK, 92);
    seq_start(); seq_tick(TSEL, CTL);
    printf("Before EDIT: sounding=%d base=%d\n", TSEL->p[P_ATK], motion_base_value(TSEL, P_ATK));
    btn_down(B_EDIT); frames(500);
    turn(EN_K2, 1);
    btn_up(B_EDIT); frame();
    hold(B_SAVE);
    seq_stop();
    printf("After sound undo and stop: attack=%d (expected 23)\n", TSEL->p[P_ATK]);
    return check("undo sound preserves original patch base under automation", TSEL->p[P_ATK] == 23);
}

static int review_storage_octave(void)
{
    ui_power_on(); frames(400);
    up_store(0, "TEST"); go_page(GR_USER); ui.uslot = 0; song.octave = 0;
    press(B_OCTDN);
    printf("USER OCT-: octave=%d (expected 0), confirm=%u\n", song.octave, ui.confirm);
    if (ui.confirm) press(B_OCTDN);
    return check("USER load/cancel keeps keyboard octave", song.octave == 0);
}

static int review_record_sound(void)
{
    ui_power_on(); frames(400);
    track_defaults_steps(TSEL);
    TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 60;
    press(B_REC); frames(400);
    TSEL->step[0].note[0] = 62; frame();
    btn_down(B_EDIT); frames(500); turn(EN_K2, 1); btn_up(B_EDIT); frame();
    TSEL->step[1].time = ST_NOTE; TSEL->step[1].n = 1; TSEL->step[1].note[0] = 65; frame();
    hold(B_SAVE);
    printf("Record mixed edit undo: note0=%u note1-count=%u what=%u\n", TSEL->step[0].note[0], TSEL->step[1].n, undo.what);
    return check("record undo after sound edit restores the take", TSEL->step[0].note[0] == 60 && TSEL->step[1].n == 0);
}

int main(void)
{
    int bad = review_automation_sound() + review_storage_octave();
    return bad != 0;
}
