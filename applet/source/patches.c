#include <stdlib.h>
#include <stdio.h>
#include <switch.h>

#include "patches.h"
#include "byteswap.h"

#define NSO_HEADER_OFFSET 0x100

typedef struct {
    const char  nso_build_id[41];
    uint32_t    offset;
    uint16_t    length;
} PatchInfo;

typedef struct __attribute__((packed)) {
    uint32_t offset;
    uint16_t length;
} ChunkHeader;

typedef struct {
    ChunkHeader header;
    uint32_t    *data;
} PatchChunk;

typedef enum {
    MOVN = 0,
    MOVZ = 2,
    MOVK = 3
} MoveWideOp;
#define MOV MOVZ

static const PatchInfo patch_info[] = { 
    {"1494B3B0E7AA4234ED20F6A5EEA60620522C68DD", 0x0000a330, 0xa}, //1.0.0
    {"874B8CD62BF6715648382BBADB70790558FB3F35", 0x0000a06c, 0xa}, //2.x.x
    {"4F57845D1DC166A46C42D5AF45164196DC7CD198", 0x0000a99c, 0xa}, //3.0.0
    {"1F66DEC82762261430EE292541FB42F5C4E10AA5", 0x0000a99c, 0xa}, //3.0.1-2
    {"DE1AC63A4A0B77CCEA606CF15B62AA587EB9CACC", 0x0000faf8, 0xa}, //4.x.x
    {"2FB16085E21E54B90E88ED2F98CCC850334E128E", 0x0001573c, 0x9}, //5.0.x
    {"F3DCA5480F0523699683C0D16B7C51C6394897AF", 0x00015a1c, 0x9}, //5.1.0
    {"A3278455B3EC297F0CE6CD629183EC86376EE0DC", 0x00017ccc, 0x9}, //6.x.x
    {"3714A43B8F9608D08E15067AB1D9CE8E68FE56BC", 0x000182DC, 0x9}, //7.x.x
    {"6FCF8BF53C10AA346555D74EC602FE53BF3BD88E", 0x000074cc, 0x9}, //8.x.x
    {"F013FF4FC88A8376A6F3C401485C0718E1B5E148", 0x0000715c, 0x9}, //9.x.x
    {"A5E6C2D20334CC787617A15BFB9F53F2DABA3E41", 0x0000759c, 0x9}, //10.x.x
};

static const char *patch_dir = "sdmc:/atmosphere/exefs_patches/bluetooth_patches";

/*
ldr     x8, [x19]
mov     x9, #<bd_addr[]>
movk    x9, #<bd_addr[]>, lsl #16
movk    x9, #<bd_addr[]>, lsl #32
mov     w10, #6
mov     x0, sp
str     x8, [sp]
stur    x9, [sp, #6]
strb    w10, [sp, #22]
*/
static uint32_t patch_instructions[] = {0x680240F9,
                                        0x0,
                                        0x0,
                                        0x0,
                                        0xCA008052,
                                        0xE0030091,
                                        0xE80300F9,
                                        0xE96300F8,
                                        0xEA5B0039};

static const uint32_t nop_instruction = 0x1F2003D5;


uint32_t encode_mov_instr(MoveWideOp opc, uint8_t Rd, uint16_t imm, uint8_t shift) {
    uint32_t sf = 0x1;
    uint32_t hw = shift / 16;
    uint32_t opcode = 0;
    opcode |= sf << 31;
    opcode |= opc << 29;
    opcode |= 0x25 << 23;
    opcode |= hw << 21;
    opcode |= imm << 5;
    opcode |= Rd << 0;
    return __bswap_32(opcode);
}

bool check_bluetooth_patches(void) {
    for (uint32_t i = 0; i < sizeof(patch_info) / sizeof(PatchInfo); ++i) {
        char patch_location[512];
        snprintf(patch_location, sizeof(patch_location), "%s/%s.ips", patch_dir, patch_info[i].nso_build_id);

        FILE *fp = fopen(patch_location, "r");
        if (fp) {
            fclose(fp);
        }
        else {
            return false;
        }
    }

  return true;
}

void init_patch_data(void) {
    Result rc;
    BluetoothAddress host_addr_reversed;
    BluetoothAdapterProperty props;

    rc = btdrvGetAdapterProperties(&props);
    if R_FAILED(rc)
        fatalThrow(rc);
        
    *(uint64_t *)&host_addr_reversed = __bswap_64(*(uint64_t *)&props.address) >> 16;
    patch_instructions[1] = encode_mov_instr(MOV, 9, *((uint16_t *)&host_addr_reversed + 0), 0x00);
    patch_instructions[2] = encode_mov_instr(MOVK, 9, *((uint16_t *)&host_addr_reversed + 1), 0x10);
    patch_instructions[3] = encode_mov_instr(MOVK, 9, *((uint16_t *)&host_addr_reversed + 2), 0x20);
}

void generate_bluetooth_patches(void) {
    init_patch_data();

    for (uint32_t i = 0; i < sizeof(patch_info) / sizeof(PatchInfo); ++i) {

        PatchChunk chunk;
        chunk.header.offset = __bswap_32(patch_info[i].offset + NSO_HEADER_OFFSET);
        chunk.header.length = __bswap_16(patch_info[i].length * sizeof(uint32_t));
        chunk.data = (uint32_t *)malloc(patch_info[i].length * sizeof(uint32_t));

        if (chunk.data != NULL) {
            for (uint32_t j = 0; j < patch_info[i].length; ++j) {
                if (j < sizeof(patch_instructions) / sizeof(uint32_t)) {
                    chunk.data[j] = __bswap_32(patch_instructions[j]);
                }
                else {
                    chunk.data[j] = __bswap_32(nop_instruction);
                }
            }

            char patch_location[512];
            snprintf(patch_location, sizeof(patch_location), "%s/%s.ips", patch_dir, patch_info[i].nso_build_id);

            FILE *fp = fopen(patch_location, "wb");
            if (fp != NULL) {
                fputs("IPS32", fp);
                fwrite(&chunk.header, sizeof(ChunkHeader), 1, fp);
                fwrite(chunk.data, sizeof(uint32_t), patch_info[i].length, fp);
                fputs("EEOF", fp);
                fclose(fp);
            }

            free(chunk.data);
        }
    }
}
