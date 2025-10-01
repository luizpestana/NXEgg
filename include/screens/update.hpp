#pragma once

#include "../fsm_screen.hpp"

#include <curl/curl.h>
#include <zzip/zzip.h>
#include <mbedtls/sha256.h>

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_set>
#include <array>
#include <dirent.h>
#include <sys/stat.h>

#include "yyjson.h"

namespace nxu
{
// ------------ file ------------

static bool file_exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static bool dir_exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool mkdirs(const std::string& path) {
    if (path.rfind("sdmc:/", 0) != 0) {
        return false;
    }
    bool ready = false;
    std::string cur;
    for (size_t i = 0; i < path.size(); ++i) {
        cur.push_back(path[i]);
        ready = cur.size() > 6;
        if (path[i] == '/' && ready) {
            ready = false;
            if (mkdir(cur.c_str(), 0777) == -1 && errno != EEXIST) {
                return false;
            }
        }
    }
    if (ready && mkdir(cur.c_str(), 0777) == -1 && errno != EEXIST) {
        return false;
    }
    return true;
}

static bool remove_tree(const std::string& dir) {
    DIR* d = opendir(dir.c_str());
    if (!d) {
        return false;
    }
    struct dirent* e;
    while ((e = readdir(d))) {
        std::string name = e->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        std::string full = dir + "/" + name;
        if (e->d_type == DT_DIR) {
            remove_tree(full);
            rmdir(full.c_str());
        } else {
            remove(full.c_str());
        }
    }
    closedir(d);
    return rmdir(dir.c_str()) == 0;
}

static bool copy_file_streaming(const std::string& src, const std::string& dst, size_t buf_sz = 128 * 1024) {
    FILE* in = fopen(src.c_str(), "rb");
    if (!in) {
        return false;
    }

    FILE* out = fopen(dst.c_str(), "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    std::vector<unsigned char> buf(buf_sz);
    size_t n;
    bool ok = true;

    while ((n = fread(buf.data(), 1, buf.size(), in)) > 0) {
        if (fwrite(buf.data(), 1, n, out) != n) {
            ok = false;
            break;
        }
    }
    if (ferror(in)) {
        ok = false;
    }

    fflush(out);
    fclose(in);
    fclose(out);
    return ok;
}

static bool copytree_shallow(const std::string& srcDir, const std::string& dstDir) {
    mkdirs(dstDir);

    DIR* d = opendir(srcDir.c_str());
    if (!d) {
        return false;
    }

    struct dirent* e;
    while ((e = readdir(d))) {
        std::string name = e->d_name;
        if (name == "." || name == "..") {
            continue;
        }

        std::string src = srcDir + "/" + name;
        std::string dst = dstDir + "/" + name;

        if (e->d_type == DT_DIR) {
            if (!copytree_shallow(src, dst)) {
                closedir(d);
                return false;
            }
        } else {
            if (!copy_file_streaming(src, dst)) {
                closedir(d);
                return false;
            }
        }
    }
    closedir(d);
    return true;
}

static bool move(const std::string& from, const std::string& to) {
    return rename(from.c_str(), to.c_str()) == 0;
}

static bool move_all_files(const std::string& srcDir, const std::string& dstDir) {
    DIR* d = opendir(srcDir.c_str());
    if (!d) {
        return false;
    }
    struct dirent* e;
    while ((e = readdir(d))) {
        std::string name = e->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        std::string src = srcDir + "/" + name;
        std::string dst = dstDir + "/" + name;

        if (e->d_type == DT_REG && ((file_exists(dst) && remove(dst.c_str()) != 0) || !move(src, dst))) {
            closedir(d);
            return false;
        }
    }
    closedir(d);
    return true;
}

static bool move_new_dirs(const std::string& srcDir, const std::string& dstDir) {
    DIR* d = opendir(srcDir.c_str());
    if (!d) {
        return false;
    }
    struct dirent* e;
    while ((e = readdir(d))) {
        std::string name = e->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        std::string src = srcDir + "/" + name;
        std::string dst = dstDir + "/" + name;

        if (e->d_type == DT_DIR && !dir_exists(dst) && !move(src, dst)) {
            closedir(d);
            return false;
        }
    }
    closedir(d);
    return true;
}

// ------------ net ------------

static bool net_connected() {
    bool connected = false;
    if (R_SUCCEEDED(nifmInitialize(NifmServiceType_User))) {
        NifmInternetConnectionStatus nifmICS;
        if (R_SUCCEEDED(nifmGetInternetConnectionStatus(NULL, NULL, &nifmICS))) {
            connected = nifmICS == NifmInternetConnectionStatus_Connected;
        }
        nifmExit();
    }
    return connected;
}

// ------------ curl ------------

struct CurlBuf {
    std::string s;
};

static size_t curl_write_str(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t n = size * nmemb;
    ((CurlBuf*)userdata)->s.append(ptr, n);
    return n;
}

static size_t curl_write_file(void* ptr, size_t size, size_t nmemb, void* stream) {
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}

static bool http_get_to_string(const std::string& url, std::string& out) {
    CURL* c = curl_easy_init();
    if (!c) {
        return false;
    }
    CurlBuf cb;
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_write_str);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &cb);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "switch-homebrew/1.0");
    curl_easy_setopt(c, CURLOPT_BUFFERSIZE, 128 * 1024L);
    curl_easy_setopt(c, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 2L);
    CURLcode rc = curl_easy_perform(c);
    curl_easy_cleanup(c);
    if (rc != CURLE_OK) {
        return false;
    }
    out.swap(cb.s);
    return true;
}

static bool http_get_to_file(const std::string& url, const std::string& path) {
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) {
        return false;
    }
    CURL* c = curl_easy_init();
    if (!c) {
        fclose(fp);
        return false;
    }
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_write_file);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "switch-homebrew/1.0");
    curl_easy_setopt(c, CURLOPT_BUFFERSIZE, 128 * 1024L);
    curl_easy_setopt(c, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 2L);
    CURLcode rc = curl_easy_perform(c);
    curl_easy_cleanup(c);
    fclose(fp);
    return (rc == CURLE_OK);
}

// ------------ unzip ------------

static bool unzip_all(const std::string& zip_path, const std::string& dest_dir) {
    zzip_error_t zerr;
    ZZIP_DIR* dir = zzip_dir_open(zip_path.c_str(), &zerr);
    if (!dir) {
        return false;
    }

    ZZIP_DIRENT ent;
    while (zzip_dir_read(dir, &ent)) {
        const char* name_c = ent.d_name;
        if (!name_c || !*name_c) {
            continue;
        }

        std::string name(name_c);
        std::string outpath = dest_dir + "/" + name;

        if (outpath.find("..") != std::string::npos) {
            continue;
        }

        if (!name.empty() && name.back() == '/') {
            if (!mkdirs(outpath)) {
                return false;
            }
            continue;
        }

        ZZIP_FILE* zf = zzip_file_open(dir, name.c_str(), 0);
        if (!zf) {
            zzip_dir_close(dir);
            return false;
        }

        FILE* out = fopen(outpath.c_str(), "wb");
        if (!out) {
            zzip_file_close(zf);
            zzip_dir_close(dir);
            return false;
        }

        char buf[64 * 1024];
        zzip_ssize_t nread;
        bool ok = true;

        while ((nread = zzip_read(zf, buf, sizeof(buf))) > 0) {
            if (fwrite(buf, 1, (size_t)nread, out) != (size_t)nread) {
                ok = false;
                break;
            }
        }
        if (nread < 0) {
            ok = false;
        }

        fclose(out);
        zzip_file_close(zf);

        if (!ok) {
            zzip_dir_close(dir);
            return false;
        }
    }

    zzip_dir_close(dir);
    return true;
}

// ------------ json ------------

struct Asset {
    std::string name;
    std::string url;
};

static bool parse_assets(const std::string& json_str, std::vector<Asset>& out) {
    yyjson_read_flag flags = YYJSON_READ_ALLOW_TRAILING_COMMAS;
    yyjson_doc *doc = yyjson_read(json_str.data(), json_str.size(), flags);
    if (!doc) {
        return false;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return false;
    }

    yyjson_val *assets = yyjson_obj_get(root, "assets");
    if (!assets || !yyjson_is_arr(assets)) {
        yyjson_doc_free(doc);
        return false;
    }

    size_t idx, max;
    yyjson_val *it;
    yyjson_arr_foreach(assets, idx, max, it) {
        if (!yyjson_is_obj(it)) {
            continue;
        }
        yyjson_val *n = yyjson_obj_get(it, "name");
        yyjson_val *u = yyjson_obj_get(it, "browser_download_url");
        const char *name = (n && yyjson_is_str(n)) ? yyjson_get_str(n) : nullptr;
        const char *url  = (u && yyjson_is_str(u)) ? yyjson_get_str(u) : nullptr;
        if (name && url) {
            out.push_back({name, url});
        }
    }

    yyjson_doc_free(doc);
    return true;
}

static bool download_assets_latest(const std::string& repoUrl, const std::vector<std::string>& fileNames, const std::string& dlDir) {
    std::string json;
    if (!http_get_to_string(repoUrl, json)) {
        return false;
    }
    std::vector<Asset> assets;
    if (!parse_assets(json, assets)) {
        return false;
    }
    const bool filtering = !fileNames.empty();
    std::unordered_set<std::string> allow;
    if (filtering) {
        allow.reserve(fileNames.size());
        for (const auto& n : fileNames) allow.insert(n);
    }

    for (auto& a : assets) {
        if (filtering && allow.find(a.name) == allow.end()) {
            continue;
        }

        std::string outPath = dlDir + "/" + a.name;
        if (!http_get_to_file(a.url, outPath)) {
            return false;
        }
        if (a.name.size() >= 4 && a.name.substr(a.name.size() - 4) == ".zip") {
            if (!unzip_all(outPath, dlDir)) {
                printf("Unzip failed");
                return false;
            }
            remove(outPath.c_str());
        }
    }
    return true;
}

// ---------- tiny SHA-256 ----------

static std::array<uint8_t,32> sha256(const uint8_t* data, size_t len) {
    std::array<uint8_t,32> out{};
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, data, len);
    mbedtls_sha256_finish_ret(&ctx, out.data());
    mbedtls_sha256_free(&ctx);
    return out;
}

// ---------- convert_boot  ----------

static bool convert_boot(const std::string& stage2_fn, const std::string& boot_fn) {
    // read stage2
    FILE* in = fopen(stage2_fn.c_str(), "rb");
    if (!in) {
        return false;
    }
    fseek(in, 0, SEEK_END);
    long sz = ftell(in);
    fseek(in, 0, SEEK_SET);
    if (sz <= 0) {
        fclose(in);
        return false;
    }
    std::vector<uint8_t> stage2((size_t)sz);
    fread(stage2.data(), 1, stage2.size(), in);
    fclose(in);

    std::vector<uint8_t> header;
    auto push = [&](const void* p, size_t n) {
        const uint8_t* b = (const uint8_t*)p;
        header.insert(header.end(), b, b+n);
    };

    // Magic "CTCaer BOOT\0"
    const uint8_t magic[] = {0x43,0x54,0x43,0x61,0x65,0x72,0x20,0x42,0x4F,0x4F,0x54,0x00};
    push(magic, sizeof(magic));
    // Version "V2.5"
    const uint8_t ver[] = {0x56,0x32,0x2E,0x35};
    push(ver, sizeof(ver));
    // SHA256(stage2)
    auto s2 = sha256(stage2.data(), stage2.size());
    push(s2.data(), s2.size());
    // Destination 0x40010000 (little-endian bytes 00 00 01 40)
    const uint8_t dest[] = {0x00,0x00,0x01,0x40};
    push(dest, sizeof(dest));
    // payload size (u32 LE)
    uint32_t s2len = (uint32_t)stage2.size();
    push(&s2len, sizeof(s2len));
    // encryption disabled = 0 (u32)
    uint32_t zero = 0; push(&zero, sizeof(zero));
    // padding 0xA4 bytes of zeros
    std::vector<uint8_t> pad(0xA4, 0x00); push(pad.data(), pad.size());
    // SHA256(header so far)
    auto h = sha256(header.data(), header.size());
    push(h.data(), h.size());

    // write out boot.dat = header + stage2
    FILE* out = fopen(boot_fn.c_str(), "wb");
    if (!out) {
        return false;
    }
    fwrite(header.data(), 1, header.size(), out);
    fwrite(stage2.data(), 1, stage2.size(), out);
    fclose(out);

    return remove(stage2_fn.c_str()) == 0;
}
}

struct UpdateScreen : FsmScreen
{
    short update;

    void entry() override {
        printf("No game is running!");
        if (nxu::net_connected()) {
            printf("\n\n");
            printf("Press A to update Atmosphere.");
            update = 0;
        } else {
            update = -1;
        }
    };

    void react(EvInputAction const &) override {
        if (update == 0) {
            update++;
            printf("\n\n");
            printf("Are you sure you want to update?");
            return;
        }

        if (update >= 2) {
            update++;
            printf("\n\n");
            printf("Restarting...");
            consoleUpdate(NULL);

            bpcInitialize();
            bpcRebootSystem();
            bpcExit();
            return;
        }

        if (update != 1) {
            return;
        }

        printf("\n\n");
        printf("Updating");
        consoleUpdate(NULL);

        socketInitializeDefault();
        romfsInit();
        curl_global_init(CURL_GLOBAL_DEFAULT);

        const std::string tmp = "sdmc:/tmp";

        if (nxu::dir_exists(tmp) && !nxu::remove_tree(tmp)) {
            printf("\n\n");
            printf("Failed to remove the existing temporary folder!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        if (!nxu::mkdirs(tmp)) {
            printf("\n\n");
            printf("Failed to create the temporary folder!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        struct RepoSpec {
            std::string url;
            std::vector<std::string> files;
        };

        std::vector<RepoSpec> repos = {
            {"https://api.github.com/repos/Atmosphere-NX/Atmosphere/releases/latest"},
            {"https://api.github.com/repos/ndeadly/MissionControl/releases/latest"},
            {"https://api.github.com/repos/impeeza/sys-patch/releases/latest"},
            {"https://api.github.com/repos/exelix11/SysDVR/releases/latest", {"SysDVR.zip"}},
            {"https://api.github.com/repos/exelix11/dvr-patches/releases/latest", {"dvr-patches.zip"}}
        };

        for (auto& repo : repos) {
            if (!nxu::download_assets_latest(repo.url, repo.files, tmp)) {
                printf("\n\n");
                printf("Failed to download the assets!");
                printf("\n");
                printf("URL: %s", repo.url.c_str());
                return;
            }
            printf(".");
            consoleUpdate(NULL);
        }

        if (!nxu::convert_boot(tmp + "/fusee.bin", tmp + "/boot.dat")) {
            printf("\n\n");
            printf("Failed to convert the boot!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        if (!nxu::move_all_files(tmp, "sdmc:/") || !nxu::move_all_files(tmp + "/switch", "sdmc:/switch")) {
            printf("\n\n");
            printf("Failed to move the files!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        if (!nxu::copytree_shallow("romfs:/patches", tmp)) {
            printf("\n\n");
            printf("Failed to copy the patches!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        if (!nxu::move("sdmc:/atmosphere", tmp + "/atmosphere_bkp")) {
            printf("\n\n");
            printf("Failed to backup Atmosphere!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        if (!nxu::move(tmp + "/atmosphere", "sdmc:/atmosphere")) {
            printf("\n\n");
            printf("Failed to move the Atmosphere!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        if (!nxu::move_new_dirs(tmp + "/atmosphere_bkp/contents", "sdmc:/atmosphere/contents")) {
            printf("\n\n");
            printf("Failed to move the Atmosphere contents!");
            return;
        }
        printf(".");
        consoleUpdate(NULL);

        if (!nxu::remove_tree(tmp)) {
            printf("\n\n");
            printf("Failed to remove the temporary folder!");
            return;
        }
        printf(".");

        printf("\n\n");
        printf("Done!");

        printf("\n\n");
        printf("Press A to reboot, X to download the latest firmware or B to exit.");
        update++;
    }

    void react(EvInputOption const &) override {
        if (update == 2) {
            update++;

            const std::string tmp = "sdmc:/tmp";

            if (nxu::dir_exists(tmp) && !nxu::remove_tree(tmp)) {
                printf("\n\n");
                printf("Failed to remove the existing temporary folder!");
                return;
            }
            printf(".");
            consoleUpdate(NULL);

            if (!nxu::mkdirs(tmp)) {
                printf("\n\n");
                printf("Failed to create the temporary folder!");
                return;
            }
            printf(".");
            consoleUpdate(NULL);

            printf("\n\n");
            printf("Downloading firmware...");
            consoleUpdate(NULL);

            if (!nxu::download_assets_latest("https://api.github.com/repos/THZoria/NX_Firmware/releases/latest", {}, tmp)) {
                printf("\n\n");
                printf("Failed to download the firmware!");
                return;
            }

            printf("\n\n");
            printf("Done!");

            printf("\n\n");
            printf("Press A to reboot or B to exit.");
        }
    }

    void react(EvInputBack const &) override {
        curl_global_cleanup();
        romfsExit();
        socketExit();
        context->Close();
    }
};
