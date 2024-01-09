#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>

#ifdef UNITTEST
    #include "unity_fixture.h"
#endif

#include "detour.h"

#define PREFIX "[DTR] "

static size_t page_size = 0;
static char states_path[255] = {0};

int dtr_fastforward(uint8_t v)
{
    uint32_t *ff = (uint32_t*)CODE_FAST_FORWARD;

    // 0xe3a03006
    mprotect(ALIGN_ADDR(CODE_FAST_FORWARD), page_size, PROT_READ | PROT_WRITE);
    *ff = 0xe3a03000 | v;
    return 0;
}

static int32_t dtr_load_state_index(void *system, uint32_t index, uint16_t *snapshot_top, uint16_t *snapshot_bottom, uint32_t snapshot_only)
{
    char buf[255] = {0};
    load_state _func = (load_state)FUN_LOAD_STATE;

    sprintf(buf, "%s/%s_%d.dss", states_path, VAR_SYSTEM_GAMECARD_NAME, index);
    _func((void*)VAR_SYSTEM, buf, snapshot_top, snapshot_bottom, snapshot_only);
}

static int32_t dtr_save_state_index(void *system, uint32_t index, uint16_t *snapshot_top, uint16_t *snapshot_bottom)
{
    char buf[255] = {0};
    save_state _func1 = (save_state)FUN_SAVE_STATE;

    sprintf(buf, "%s_%d.dss", VAR_SYSTEM_GAMECARD_NAME, index);
    _func1((void*)VAR_SYSTEM, states_path, buf, snapshot_top, snapshot_bottom);
}

static void dtr_initialize_backup(backup_struct *backup, backup_type_enum backup_type, uint8_t *data, uint32_t size, char *path)
{
    char *data_file_name = NULL;
    FILE *__stream = NULL;
    FILE *data_file = NULL;
    size_t sVar1 = 0;
    long __off = 0;
    uint32_t uVar2 = 0;
    uint32_t backup_file_size = 0;
    uint32_t uVar3 = 0;
    void *pvVar4 = NULL;
    uint8_t *desmume_footer_ptr = NULL;
    uint8_t uVar5 = 0;
    uint32_t truncate_size = 0;
    uint32_t desmume_footer_position = 0;
    uint32_t clean_pages_loaded = 0;

    if (path != NULL) {
        data_file_name = malloc(255);
        memset(data_file_name, 0, 255);
        sprintf(data_file_name, "%s/%s.dsv", states_path, VAR_SYSTEM_GAMECARD_NAME);
    }
    backup->type = backup_type;
    backup->address_mask = size - 1;
    backup->fix_file_size = 0;
    backup->data = data;
    backup->firmware = 0;
    if (backup_type != BACKUP_TYPE_FLASH) {
        if (backup_type == BACKUP_TYPE_NONE) {
            backup->address_bytes = 0;
            goto LAB_08092f94;
        }

        if (backup_type != BACKUP_TYPE_EEPROM) {
            goto LAB_08092f94;
        }

        if (size < 0x10001) {
            uVar5 = 0x02;
            if (size < 0x201) {
                uVar5 = 0x01;
            }
            backup->address_bytes = uVar5;
            goto LAB_08092f94;
        }
    }
    backup->address_bytes = 3;

LAB_08092f94:
    if (data_file_name == (char *)0x0) {
        backup->file_path[0] = 0;
    }
    else {
        __stream = fopen(data_file_name,"rb");
        backup->footer_written = '\0';
        if (__stream == (FILE *)0x0) {
            printf(PREFIX"Failed to load backup file.");
            memset(data, size, 0xff);
            memset(backup, 0xff, size >> 0xc);
        }
        else {
            sVar1 = fread(data, size, 1, __stream);
            if (sVar1 != 1) {
                printf(PREFIX"Failed to load entire size.");
            }

            __off = ftell(__stream);
            fseek(__stream, 0, 2);
            uVar2 = ftell(__stream);
            fseek(__stream, __off, 0);
            fclose(__stream);
            printf(PREFIX"Loading backup file %s, %d bytes\n", data_file_name, uVar2);

            if (size + 0x7a != uVar2) {
                backup->fix_file_size = size + 0x7a;
            }

            if (uVar2 < size) {
                uVar3 = uVar2 - 0x400 & ~((int)(uVar2 - 0x400) >> 0x1f);
                pvVar4 = memmem(data + uVar3,uVar2 - uVar3, (const void *)VAR_DESMUME_FOOTER_STR, 0x52);
                if (pvVar4 != (void *)0x0) {
                    uVar2 = (int)pvVar4 - (int)data;
                    printf(PREFIX"Found DeSmuME footer at position %d. Truncating.\n", uVar2);
                }
                uVar3 = uVar2 >> 0xe;
                printf(PREFIX"Backup file less than full size (should be %d, loaded %d).\n", size, uVar2);
                memset(data + uVar2, size - uVar2, 0xff);
                memset(backup, 0, uVar3 * 4);
                memset(backup->dirty_page_bitmap + uVar3, 0xff, ((size + 0x3fff >> 0xe) - uVar3) * 4);
            }
            else {
                memset(backup, 0, size + 0x3fff >> 0xe);
            }
        }
        strncpy(backup->file_path, data_file_name, 0x3ff);
        backup->file_path[0x3ff] = 0;
    }
    backup->status = 0;

    if (data_file_name) {
        free(data_file_name);
    }
}

int dtr_savestate(int slot)
{
#ifdef MMIYOO
    char buf[255] = {0};
    screen_copy16 _func0 = (screen_copy16)FUN_SCREEN_COPY16;

    void *d0 = malloc(0x18000);
    void *d1 = malloc(0x18000);

    if ((d0 != NULL) && (d1 != NULL)) {
        _func0(d0, 0);
        _func0(d1, 1);

        if (states_path[0] == 0) {
            save_state_index _func1 = (save_state_index)FUN_SAVE_STATE_INDEX;

            _func1((void*)VAR_SYSTEM, slot, d0, d1);
        }
        else {
            save_state _func1 = (save_state)FUN_SAVE_STATE;

            sprintf(buf, "%s_%d.dss", VAR_SYSTEM_GAMECARD_NAME, slot);
            _func1((void*)VAR_SYSTEM, states_path, buf, d0, d1);
        }
    }
    if (d0 != NULL) {
        free(d0);
    }
    if (d1 != NULL) {
        free(d1);
    }
#endif

#if defined(TRIMUI) || defined(FUNKEYS) || defined(PANDORA)
    screen_copy16 _func0 = (screen_copy16)FUN_SCREEN_COPY16;
    save_state_index _func1 = (save_state_index)FUN_SAVE_STATE_INDEX;

    void *d0 = malloc(0x18000);
    void *d1 = malloc(0x18000);

    if ((d0 != NULL) && (d1 != NULL)) {
        _func0(d0, 0);
        _func0(d1, 1);
        _func1((void*)VAR_SYSTEM, slot, d0, d1);
    }
    if (d0 != NULL) {
        free(d0);
    }
    if (d1 != NULL) {
        free(d1);
    }
#endif
}

int dtr_loadstate(int slot)
{
#ifdef MMIYOO
    char buf[255] = {0};

    if (states_path[0] == 0) {
        load_state_index _func = (load_state_index)FUN_LOAD_STATE_INDEX;

        _func((void*)VAR_SYSTEM, slot, 0, 0, 0);
    }
    else {
        load_state _func = (load_state)FUN_LOAD_STATE;

        sprintf(buf, "%s/%s_%d.dss", states_path, VAR_SYSTEM_GAMECARD_NAME, slot);
        _func((void*)VAR_SYSTEM, buf, 0, 0, 0);
    }
#endif

#if defined(TRIMUI) || defined(FUNKEYS) || defined(PANDORA)
    load_state_index _func = (load_state_index)FUN_LOAD_STATE_INDEX;

    _func((void*)VAR_SYSTEM, slot, 0, 0, 0);
#endif
}

int dtr_quit(void)
{
    quit _func = (quit)FUN_QUIT;

    _func((void*)VAR_SYSTEM);
}

void detour_init(size_t page, const char *path)
{
    page_size = page;

    if ((path != NULL) && (path[0] != 0)) {
        strcpy(states_path, path);
        detour_hook(FUN_LOAD_STATE_INDEX, (intptr_t)dtr_load_state_index);
        detour_hook(FUN_SAVE_STATE_INDEX, (intptr_t)dtr_save_state_index);
        detour_hook(FUN_INITIALIZE_BACKUP, (intptr_t)dtr_initialize_backup);
        printf(PREFIX"Enabled hooking\n");
    }
    else {
        printf(PREFIX"Disabled hooking\n");
    }
}

void detour_hook(uint32_t old_func, uint32_t new_func)
{
    volatile uint8_t *base = (uint8_t *)(intptr_t)old_func;

    mprotect(ALIGN_ADDR(base), page_size, PROT_READ | PROT_WRITE);
    base[0] = 0x04;
    base[1] = 0xf0;
    base[2] = 0x1f;
    base[3] = 0xe5;
    base[4] = new_func >> 0;
    base[5] = new_func >> 8;
    base[6] = new_func >> 16;
    base[7] = new_func >> 24;
}

void detour_quit(void)
{
}

#ifdef UNITTEST
TEST_GROUP(detour);

TEST_SETUP(detour)
{
}

TEST_TEAR_DOWN(detour)
{
}

TEST(detour, detour_init)
{
    detour_init(0, "/tmp");
    TEST_ASSERT_EQUAL_INT(page_size, 0);
    TEST_ASSERT_EQUAL_STRING(states_path, "/tmp");
}

TEST_GROUP_RUNNER(detour)
{
    RUN_TEST_CASE(detour, detour_init);
}
#endif

