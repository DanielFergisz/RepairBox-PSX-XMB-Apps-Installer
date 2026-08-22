#include <errno.h>
#include <hdd-ioctl.h>
#include <stdio.h>
#include <string.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#include "app_installer.h"
#include "sha256.h"

#define APA_MAGIC 0x00415041u
#define APA_HEADER_BYTES 1024u
#define SECTOR_BYTES 512u
#define APP_PARTITION_SECTORS 0x00040000u
#define APP_PFS_ZONE_SIZE 8192
#define APP_STEP_COUNT 6u
#define COPY_BUFFER_SIZE (64u * 1024u)

typedef struct apa_time {
    u8 unused, sec, min, hour, day, month;
    u16 year;
} apa_time_t;

typedef struct apa_sub {
    u32 start;
    u32 length;
} apa_sub_t;

typedef struct apa_header_public {
    u32 checksum, magic, next, prev;
    char id[32], rpwd[8], fpwd[8];
    u32 start, length;
    u16 type, flags;
    u32 nsub;
    apa_time_t created;
    u32 main, number, modver, padding1[7];
    char padding2[128];
    struct {
        char magic[32];
        u32 version, nsector;
        apa_time_t created;
        u32 osd_start, osd_size;
        char padding3[200];
    } mbr;
    apa_sub_t subs[64];
} apa_header_public_t;

static const app_package_file_t wle_files[APP_FILE_COUNT] = {
    {"EXECUTE.KELF", 433864u,
     "028FC3931F9906754AC3F28D35B13282D71B5228A8BACF17B8BF86FD6E1E8B5B"},
    {"SYSTEM.CNF", 76u,
     "03AD5276E94683381919C3855241DA0981EBFED0102C64EA520990A91D0213AB"},
    {"icon.sys", 336u,
     "337CC7C91DFE995008726A6B71F2BAC2A7F3DCA1FF1F7C8CF5D4236C128BDBDC"},
    {"res/info.sys", 477u,
     "2EE4A295BA1EFEB9A3A2558838D6E1B3405AF2A3DBF4B2251FE89BE4F16D816D"},
    {"res/jkt_001.png", 2677u,
     "C10DDF15A06CF33E6153A5BE59BE574A473D68F3E888100A40818F7A106D946E"},
    {"res/jkt_002.png", 2677u,
     "C10DDF15A06CF33E6153A5BE59BE574A473D68F3E888100A40818F7A106D946E"},
};

static const app_package_file_t opl_files[APP_FILE_COUNT] = {
    {"EXECUTE.KELF", 1360616u,
     "CC49314F71DF67E1E3CC014DD13D4D23AC2A17789C03023625AD405AD075EB2E"},
    {"SYSTEM.CNF", 76u,
     "03AD5276E94683381919C3855241DA0981EBFED0102C64EA520990A91D0213AB"},
    {"icon.sys", 337u,
     "81215EFEDBC796D3CBDF7958D2D5CDB72739E8C2E20D798CD58FF3FED2DBE627"},
    {"res/info.sys", 457u,
     "B3A88673D908A9D7B1AC10BEAB1562A486C11A681959701DC50157261067E094"},
    {"res/jkt_001.png", 1143u,
     "341366442BF133B05CA0BBC2C94B053FAD2F16C842D0BDDB7E1414D46E49CE1B"},
    {"res/jkt_002.png", 1143u,
     "341366442BF133B05CA0BBC2C94B053FAD2F16C842D0BDDB7E1414D46E49CE1B"},
};

static const app_package_file_t padtest_files[APP_FILE_COUNT] = {
    {"EXECUTE.KELF", 199192u,
     "FCFA6F3BA590ECEE2C2D7E284076FF4F54CEB62128DD98A64FD32BE1C17B68AB"},
    {"SYSTEM.CNF", 76u,
     "03AD5276E94683381919C3855241DA0981EBFED0102C64EA520990A91D0213AB"},
    {"icon.sys", 325u,
     "F555840EEC8D03BE98E9836051EFAD63F72DD5CE5B946BC5E6EF9F41406A83E1"},
    {"res/info.sys", 452u,
     "100945B15226835E69538C232B8C55C59326191D78627B696B455DF01211C8E8"},
    {"res/jkt_001.png", 1186u,
     "EF053A4FCEDB0E41C4565745A309D708FC518110DC7877BA9ED9F3F4634860C2"},
    {"res/jkt_002.png", 1186u,
     "EF053A4FCEDB0E41C4565745A309D708FC518110DC7877BA9ED9F3F4634860C2"},
};

static const app_package_file_t popsloader_files[APP_FILE_COUNT] = {
    {"EXECUTE.KELF", 1332552u,
     "33D464655D0C8DDA3DE3128E12B66E22FF43D52556B6BC16D6DD508B19FB0B78"},
    {"SYSTEM.CNF", 76u,
     "03AD5276E94683381919C3855241DA0981EBFED0102C64EA520990A91D0213AB"},
    {"icon.sys", 331u,
     "021D98B7D8B4C4604DFB0F6B42350A713E54A34ADC3913549EA7C406171846E8"},
    {"res/info.sys", 461u,
     "35DB8E60F885615806904C1236EBB8C35877D1E4B7313C57A3C6C42AB6E57A27"},
    {"res/jkt_001.png", 1295u,
     "33D97E65C9FD65718957436C20A7DD453D6960E398019E917ED1E4790C368CA2"},
    {"res/jkt_002.png", 1295u,
     "33D97E65C9FD65718957436C20A7DD453D6960E398019E917ED1E4790C368CA2"},
};

const app_package_t app_packages[APP_COUNT] = {
    {"WLE-R3Z-DS34", "wLaunchELF R3Z", "PP.APPS-00002..WLE",
     "mass:/PSX_XMB_Apps/WLE-R3Z-DS34", wle_files},
    {"OPL", "Open PS2 Loader", "PP.APPS-00003..OPL",
     "mass:/PSX_XMB_Apps/OPL", opl_files},
    {"PADTEST", "PadTest", "PP.APPS-00004..PADTEST",
     "mass:/PSX_XMB_Apps/PADTEST", padtest_files},
    {"POPSLOADER", "POPSLoader", "PP.APPS-00005..POPS",
     "mass:/PSX_XMB_Apps/POPSLOADER", popsloader_files},
};

_Static_assert(sizeof(apa_header_public_t) == APA_HEADER_BYTES,
               "APA header layout must be 1024 bytes");

static unsigned char copy_buffer[COPY_BUFFER_SIZE]
    __attribute__((aligned(64)));
static unsigned char apa_buffer[APA_HEADER_BYTES]
    __attribute__((aligned(64)));
static unsigned char readback_sector[SECTOR_BYTES]
    __attribute__((aligned(64)));
static unsigned char write_transfer_buffer[sizeof(hddAtaTransfer_t) +
                                           SECTOR_BYTES]
    __attribute__((aligned(64)));

static void make_source_path(const app_package_t *package,
                             char *output, size_t size,
                             const char *relative_path)
{
    snprintf(output, size, "%s/%s", package->source_root, relative_path);
}

static void make_destination_path(char *output, size_t size,
                                  const char *relative_path)
{
    snprintf(output, size, "pfs0:/%s", relative_path);
}

static int hash_file(const char *path, u64 *size, char hex[65],
                     u64 *work_done, u64 total_work,
                     app_progress_callback_t progress, void *context)
{
    sha256_context_t hash;
    unsigned char digest[SHA256_DIGEST_SIZE];
    int fd = fileXioOpen(path, FIO_O_RDONLY, 0);

    if (fd < 0)
        return fd;
    *size = 0;
    sha256_init(&hash);
    for (;;) {
        int read_result = fileXioRead(fd, copy_buffer,
                                     sizeof(copy_buffer));

        if (read_result < 0) {
            fileXioClose(fd);
            return read_result;
        }
        if (read_result == 0)
            break;
        sha256_update(&hash, copy_buffer, (size_t)read_result);
        *size += (u64)read_result;
        if (work_done != NULL) {
            *work_done += (u64)read_result;
            progress(3, APP_STEP_COUNT, "INSTALL AND VERIFY FILES",
                     total_work == 0 ? 0u :
                         (unsigned int)(*work_done * 100u / total_work),
                     context);
        }
    }
    {
        int close_result = fileXioClose(fd);

        if (close_result < 0)
            return close_result;
    }
    sha256_final(&hash, digest);
    sha256_to_hex(digest, hex);
    return 0;
}

static int read_sectors(u32 lba, u32 count, void *output)
{
    hddAtaTransfer_t request;

    request.lba = lba;
    request.size = count;
    return fileXioDevctl("hdd0:", HDIOC_READSECTOR,
                         &request, sizeof(request), output,
                         count * SECTOR_BYTES);
}

static int write_sector(u32 lba, const void *data)
{
    hddAtaTransfer_t *transfer =
        (hddAtaTransfer_t *)write_transfer_buffer;

    transfer->lba = lba;
    transfer->size = 1;
    memcpy(transfer->data, data, SECTOR_BYTES);
    return fileXioDevctl("hdd0:", HDIOC_WRITESECTOR, transfer,
                         sizeof(*transfer) + SECTOR_BYTES, NULL, 0);
}

static int apa_checksum_valid(const apa_header_public_t *header)
{
    const u32 *words = (const u32 *)header;
    u32 checksum = 0;
    unsigned int index;

    for (index = 1; index < APA_HEADER_BYTES / sizeof(u32); ++index)
        checksum += words[index];
    return header->magic == APA_MAGIC && header->checksum == checksum;
}

static int hash_text_equal(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        unsigned char a = (unsigned char)*left++;
        unsigned char b = (unsigned char)*right++;

        if (a >= 'A' && a <= 'F')
            a = (unsigned char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'F')
            b = (unsigned char)(b - 'A' + 'a');
        if (a != b)
            return 0;
    }
    return *left == '\0' && *right == '\0';
}

static void initialize_file_diagnostic(const app_package_t *package,
                                       app_file_diagnostic_t *diagnostic,
                                       unsigned int index)
{
    memset(diagnostic, 0, sizeof(*diagnostic));
    snprintf(diagnostic->relative_path, sizeof(diagnostic->relative_path),
             "%s", package->files[index].relative_path);
    diagnostic->expected_size = package->files[index].size;
    snprintf(diagnostic->expected_sha256,
             sizeof(diagnostic->expected_sha256), "%s",
             package->files[index].sha256);
}

static int validate_target_header(const app_package_t *package,
                                  u32 start, u32 expected_length)
{
    const apa_header_public_t *header =
        (const apa_header_public_t *)apa_buffer;
    int result = read_sectors(start, 2, apa_buffer);

    if (result < 0)
        return result;
    if (!apa_checksum_valid(header) ||
        strncmp(header->id, package->partition_name,
                sizeof(header->id)) != 0 ||
        header->start != start || header->length != expected_length ||
        header->type != 0x0100u || header->nsub != 0)
        return -EINVAL;
    return 0;
}

static int scan_hdd_layout(const app_package_t *package,
                           app_preflight_t *result)
{
    static const char *const required[] = {
        "__mbr", "__net", "__system", "__sysconf", "__common"};
    unsigned int found_mask = 0;
    iox_dirent_t entry;
    int fd = fileXioDopen("hdd0:");
    int read_result = fd;

    result->hdd_open_result = fd;
    if (fd < 0)
        return fd;
    for (;;) {
        unsigned int index;

        memset(&entry, 0, sizeof(entry));
        read_result = fileXioDread(fd, &entry);
        if (read_result <= 0)
            break;
        for (index = 0; index < sizeof(required) / sizeof(required[0]);
             ++index) {
            if (strcmp(entry.name, required[index]) == 0)
                found_mask |= 1u << index;
        }
        if (strcmp(entry.name, package->partition_name) == 0) {
            if (result->target_exists) {
                read_result = -EINVAL;
                break;
            }
            result->target_exists = 1;
            result->target_start = entry.stat.private_5;
            result->target_length = entry.stat.size;
        }
    }
    result->hdd_close_result = fileXioDclose(fd);
    if (read_result < 0)
        return read_result;
    if (result->hdd_close_result < 0)
        return result->hdd_close_result;
    result->required_partitions_valid =
        found_mask == (1u << (sizeof(required) / sizeof(required[0]))) - 1u;
    if (!result->required_partitions_valid)
        return -EINVAL;
    if (result->target_exists) {
        if (result->target_length != APP_PARTITION_SECTORS)
            return -EINVAL;
        return validate_target_header(package, result->target_start,
                                      result->target_length);
    }
    return 0;
}

void app_scan_preflight(const app_package_t *package,
                        app_preflight_t *result)
{
    unsigned int index;

    memset(result, 0, sizeof(*result));
    result->package = package;
    for (index = 0; index < APP_FILE_COUNT; ++index)
        initialize_file_diagnostic(package, &result->files[index], index);
    result->hdd_status =
        fileXioDevctl("hdd0:", HDIOC_STATUS, NULL, 0, NULL, 0);
    result->format_version =
        fileXioDevctl("hdd0:", HDIOC_FORMATVER, NULL, 0, NULL, 0);
    if (result->hdd_status < 0 || result->format_version < 0) {
        result->failure_result = result->hdd_status < 0
            ? result->hdd_status : result->format_version;
        snprintf(result->failure_item, sizeof(result->failure_item),
                 "hdd0 status/format version");
        return;
    }
    result->failure_result = scan_hdd_layout(package, result);
    if (result->failure_result < 0) {
        snprintf(result->failure_item, sizeof(result->failure_item),
                 "hdd0 layout");
        return;
    }
    for (index = 0; index < APP_FILE_COUNT; ++index) {
        char source[192];
        app_file_diagnostic_t *diagnostic = &result->files[index];
        int hash_result;

        make_source_path(package, source, sizeof(source),
                         package->files[index].relative_path);
        hash_result = hash_file(source, &diagnostic->source_size,
                                diagnostic->source_sha256,
                                NULL, 0, NULL, NULL);
        diagnostic->source_hash_result = hash_result;
        diagnostic->source_valid =
            hash_result >= 0 &&
            diagnostic->source_size == package->files[index].size &&
            hash_text_equal(diagnostic->source_sha256,
                            package->files[index].sha256);
        if (!diagnostic->source_valid) {
            result->failure_result = hash_result < 0 ? hash_result : -EIO;
            snprintf(result->failure_item, sizeof(result->failure_item),
                     "%s", package->files[index].relative_path);
            return;
        }
        ++result->package_files_valid;
        result->package_bytes += diagnostic->source_size;
    }
    result->package_valid = 1;
}

static void set_failure(app_install_result_t *result, unsigned int step,
                        int value, const char *operation,
                        const char *item)
{
    if (result->failure_result != 0)
        return;
    result->failed_step = (int)step;
    result->failure_result = value < 0 ? value : -EIO;
    snprintf(result->failure_operation, sizeof(result->failure_operation),
             "%s", operation);
    snprintf(result->failure_item, sizeof(result->failure_item), "%s", item);
}

static int refresh_target(const app_package_t *package,
                          app_install_result_t *result)
{
    iox_dirent_t entry;
    int fd = fileXioDopen("hdd0:");
    int read_result = fd;
    int found = 0;

    if (fd < 0)
        return fd;
    for (;;) {
        memset(&entry, 0, sizeof(entry));
        read_result = fileXioDread(fd, &entry);
        if (read_result <= 0)
            break;
        if (strcmp(entry.name, package->partition_name) == 0) {
            result->target_start = entry.stat.private_5;
            result->target_length = entry.stat.size;
            found = 1;
        }
    }
    {
        int close_result = fileXioDclose(fd);

        if (read_result >= 0 && close_result < 0)
            read_result = close_result;
    }
    if (read_result < 0)
        return read_result;
    if (!found || result->target_length != APP_PARTITION_SECTORS)
        return -EINVAL;
    return validate_target_header(package, result->target_start,
                                  result->target_length);
}

static int create_target_partition(const app_package_t *package)
{
    char target[96];
    int fd;

    snprintf(target, sizeof(target), "hdd0:%s,,,128M,PFS",
             package->partition_name);
    fd = fileXioOpen(target, FIO_O_RDWR | FIO_O_CREAT, 0);

    if (fd < 0)
        return fd;
    return fileXioClose(fd);
}

static int ensure_directory(const char *path)
{
    int result = fileXioMkdir(path, 0777);

    if (result >= 0)
        return 0;
    {
        int fd = fileXioDopen(path);

        if (fd < 0)
            return result;
        return fileXioDclose(fd);
    }
}

static int copy_and_verify_files(const app_package_t *package,
                                 app_install_result_t *result,
                                 app_progress_callback_t progress,
                                 void *context)
{
    u64 total_work = 0;
    u64 work_done = 0;
    unsigned int index;
    int directory_result;

    for (index = 0; index < APP_FILE_COUNT; ++index)
        total_work += package->files[index].size * 2u;
    directory_result = ensure_directory("pfs0:/res");
    if (directory_result < 0) {
        set_failure(result, 3, directory_result, "create directory", "res");
        return directory_result;
    }

    progress(3, APP_STEP_COUNT, "INSTALL AND VERIFY FILES", 0, context);
    for (index = 0; index < APP_FILE_COUNT; ++index) {
        char source[192];
        char destination[192];
        app_file_diagnostic_t *diagnostic = &result->files[index];
        int source_fd = -1;
        int destination_fd = -1;
        int return_value = 0;

        make_source_path(package, source, sizeof(source),
                         package->files[index].relative_path);
        make_destination_path(destination, sizeof(destination),
                              package->files[index].relative_path);
        source_fd = fileXioOpen(source, FIO_O_RDONLY, 0);
        diagnostic->source_open_result = source_fd;
        if (source_fd < 0) {
            set_failure(result, 3, source_fd, "open source",
                        package->files[index].relative_path);
            return source_fd;
        }
        destination_fd = fileXioOpen(
            destination, FIO_O_WRONLY | FIO_O_CREAT | FIO_O_TRUNC, 0666);
        diagnostic->destination_open_result = destination_fd;
        if (destination_fd < 0) {
            diagnostic->source_close_result = fileXioClose(source_fd);
            set_failure(result, 3, destination_fd, "open destination",
                        package->files[index].relative_path);
            return destination_fd;
        }
        for (;;) {
            int read_result = fileXioRead(source_fd, copy_buffer,
                                         sizeof(copy_buffer));
            int written = 0;

            diagnostic->source_read_result = read_result;
            if (read_result < 0) {
                return_value = read_result;
                set_failure(result, 3, return_value, "read source",
                            package->files[index].relative_path);
                break;
            }
            if (read_result == 0)
                break;
            while (written < read_result) {
                int write_result = fileXioWrite(
                    destination_fd, copy_buffer + written,
                    read_result - written);

                diagnostic->destination_write_result = write_result;
                if (write_result <= 0) {
                    return_value = write_result < 0 ? write_result : -EIO;
                    set_failure(result, 3, return_value,
                                "write destination",
                                package->files[index].relative_path);
                    break;
                }
                written += write_result;
            }
            if (return_value < 0)
                break;
            work_done += (u64)read_result;
            result->copied_bytes += (u64)read_result;
            progress(3, APP_STEP_COUNT, "INSTALL AND VERIFY FILES",
                     (unsigned int)(work_done * 100u / total_work), context);
        }
        {
            int close_result = fileXioClose(source_fd);

            diagnostic->source_close_result = close_result;
            if (return_value >= 0 && close_result < 0)
                return_value = close_result;
            if (close_result < 0)
                set_failure(result, 3, close_result, "close source",
                            package->files[index].relative_path);
        }
        {
            int close_result = fileXioClose(destination_fd);

            diagnostic->destination_close_result = close_result;
            if (return_value >= 0 && close_result < 0)
                return_value = close_result;
            if (close_result < 0)
                set_failure(result, 3, close_result,
                            "close destination",
                            package->files[index].relative_path);
        }
        if (return_value < 0)
            return return_value;
        ++result->copied_files;
        return_value = hash_file(
            destination, &diagnostic->target_size,
            diagnostic->target_sha256,
            &work_done, total_work, progress, context);
        diagnostic->target_hash_result = return_value;
        if (return_value < 0) {
            set_failure(result, 3, return_value, "hash destination",
                        package->files[index].relative_path);
            return return_value;
        }
        diagnostic->target_valid =
            diagnostic->target_size == package->files[index].size &&
            hash_text_equal(diagnostic->target_sha256,
                            package->files[index].sha256);
        if (!diagnostic->target_valid) {
            set_failure(result, 3, -EIO, "verify destination",
                        package->files[index].relative_path);
            return -EIO;
        }
        ++result->verified_files;
    }
    progress(3, APP_STEP_COUNT, "INSTALL AND VERIFY FILES", 100, context);
    return 0;
}

static int read_small_source(const app_package_t *package,
                             const char *relative_path,
                             unsigned char *output, size_t capacity,
                             u32 *output_size)
{
    char source[192];
    int fd;
    int read_result;
    int close_result;

    make_source_path(package, source, sizeof(source), relative_path);
    fd = fileXioOpen(source, FIO_O_RDONLY, 0);
    if (fd < 0)
        return fd;
    read_result = fileXioRead(fd, output, capacity);
    close_result = fileXioClose(fd);
    if (read_result < 0)
        return read_result;
    if (close_result < 0)
        return close_result;
    *output_size = (u32)read_result;
    return 0;
}

static void set_le32(unsigned char *output, u32 value)
{
    output[0] = (unsigned char)value;
    output[1] = (unsigned char)(value >> 8);
    output[2] = (unsigned char)(value >> 16);
    output[3] = (unsigned char)(value >> 24);
}

static int register_xmb_entry(const app_package_t *package,
                              u32 partition_start)
{
    unsigned char ppaa[SECTOR_BYTES] __attribute__((aligned(64)));
    unsigned char system_cnf[SECTOR_BYTES] __attribute__((aligned(64)));
    unsigned char icon_sys[SECTOR_BYTES] __attribute__((aligned(64)));
    u32 system_size = 0;
    u32 icon_size = 0;
    int result;

    memset(ppaa, 0, sizeof(ppaa));
    memset(system_cnf, 0, sizeof(system_cnf));
    memset(icon_sys, 0, sizeof(icon_sys));
    result = read_small_source(package, "SYSTEM.CNF", system_cnf,
                               sizeof(system_cnf), &system_size);
    if (result < 0)
        return result;
    result = read_small_source(package, "icon.sys", icon_sys,
                               sizeof(icon_sys), &icon_size);
    if (result < 0)
        return result;
    if (system_size == 0 || icon_size == 0 ||
        memcmp(icon_sys, "PS2X", 4) != 0)
        return -EINVAL;

    memcpy(ppaa, "PS2ICON3D", 9);
    set_le32(ppaa + 0x10, 0x0200u);
    set_le32(ppaa + 0x14, system_size);
    set_le32(ppaa + 0x18, 0x0400u);
    set_le32(ppaa + 0x1c, icon_size);

    result = write_sector(partition_start + 8u, ppaa);
    if (result < 0)
        return result;
    result = write_sector(partition_start + 9u, system_cnf);
    if (result < 0)
        return result;
    result = write_sector(partition_start + 10u, icon_sys);
    if (result < 0)
        return result;

    result = read_sectors(partition_start + 8u, 1, readback_sector);
    if (result < 0 || memcmp(readback_sector, ppaa, SECTOR_BYTES) != 0)
        return result < 0 ? result : -EIO;
    result = read_sectors(partition_start + 9u, 1, readback_sector);
    if (result < 0 ||
        memcmp(readback_sector, system_cnf, SECTOR_BYTES) != 0)
        return result < 0 ? result : -EIO;
    result = read_sectors(partition_start + 10u, 1, readback_sector);
    if (result < 0 || memcmp(readback_sector, icon_sys, SECTOR_BYTES) != 0)
        return result < 0 ? result : -EIO;
    return 0;
}

void app_install(const app_package_t *package,
                 psx_revision_t revision,
                 const app_preflight_t *preflight,
                 app_install_result_t *result,
                 app_progress_callback_t progress,
                 void *context)
{
    static const int pfs_format_args[1] = {APP_PFS_ZONE_SIZE};
    char blockdev[80];
    int return_value;
    int mount_result;
    int mount_owned = 0;

    (void)revision;
    memset(result, 0, sizeof(*result));
    result->package = package;
    snprintf(blockdev, sizeof(blockdev), "hdd0:%s",
             package->partition_name);
    memcpy(result->files, preflight->files, sizeof(result->files));
    if (!preflight->package_valid || !preflight->required_partitions_valid) {
        set_failure(result, 1, -EPERM, "preflight", "package/layout");
        return;
    }

    progress(1, APP_STEP_COUNT, "PREPARE APPLICATION PARTITION", 0,
             context);
    if (!preflight->target_exists) {
        return_value = create_target_partition(package);
        if (return_value < 0) {
            set_failure(result, 1, return_value, "create partition",
                        package->partition_name);
            return;
        }
        result->target_was_created = 1;
    }
    return_value = refresh_target(package, result);
    if (return_value < 0) {
        set_failure(result, 1, return_value, "validate APA target",
                    package->partition_name);
        return;
    }
    progress(1, APP_STEP_COUNT, "PREPARE APPLICATION PARTITION", 100,
             context);

    progress(2, APP_STEP_COUNT, "PREPARE PFS FILESYSTEM", 0, context);
    fileXioUmount("pfs0:");
    mount_result = fileXioMount("pfs0:", blockdev, FIO_MT_RDWR);
    if (mount_result < 0) {
        return_value = fileXioFormat(
            "pfs:", blockdev, (const char *)&pfs_format_args,
            sizeof(pfs_format_args));
        if (return_value < 0) {
            set_failure(result, 2, return_value, "format app PFS",
                        package->partition_name);
            return;
        }
        result->target_was_reformatted = 1;
        mount_result = fileXioMount("pfs0:", blockdev, FIO_MT_RDWR);
    }
    if (mount_result < 0) {
        set_failure(result, 2, mount_result, "mount app PFS",
                    package->partition_name);
        return;
    }
    mount_owned = 1;
    progress(2, APP_STEP_COUNT, "PREPARE PFS FILESYSTEM", 100, context);

    return_value = copy_and_verify_files(package, result, progress, context);
    if (return_value < 0) {
        if (result->failure_result == 0)
            set_failure(result, 3, return_value, "copy/verify package",
                        package->partition_name);
        goto cleanup;
    }

    progress(4, APP_STEP_COUNT, "FLUSH AND CLOSE FILESYSTEM", 0, context);
    return_value = fileXioSync("pfs0:", FXIO_WAIT);
    if (return_value < 0) {
        set_failure(result, 4, return_value, "sync PFS",
                    package->partition_name);
        goto cleanup;
    }
    return_value = fileXioUmount("pfs0:");
    mount_owned = 0;
    if (return_value < 0) {
        set_failure(result, 4, return_value, "unmount PFS",
                    package->partition_name);
        return;
    }
    progress(4, APP_STEP_COUNT, "FLUSH AND CLOSE FILESYSTEM", 100,
             context);

    progress(5, APP_STEP_COUNT, "REGISTER APPLICATION IN XMB", 0,
             context);
    return_value = register_xmb_entry(package, result->target_start);
    if (return_value < 0) {
        set_failure(result, 5, return_value, "write/read XMB header",
                    package->partition_name);
        return;
    }
    progress(5, APP_STEP_COUNT, "REGISTER APPLICATION IN XMB", 100,
             context);

    progress(6, APP_STEP_COUNT, "FINAL TARGET VERIFICATION", 0, context);
    return_value = validate_target_header(package, result->target_start,
                                          result->target_length);
    if (return_value < 0 || result->copied_files != APP_FILE_COUNT ||
        result->verified_files != APP_FILE_COUNT) {
        set_failure(result, 6, return_value < 0 ? return_value : -EIO,
                    "final verification", package->partition_name);
        return;
    }
    progress(6, APP_STEP_COUNT, "FINAL TARGET VERIFICATION", 100,
             context);
    result->success = 1;
    return;

cleanup:
    if (mount_owned)
        fileXioUmount("pfs0:");
}
