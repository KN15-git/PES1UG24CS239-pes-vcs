// index.c — Staging area implementation

#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

// ─── PROVIDED ────────────────────────────────────────────────────────────────

IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int remaining = index->count - i - 1;
            if (remaining > 0)
                memmove(&index->entries[i], &index->entries[i + 1],
                        remaining * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    fprintf(stderr, "error: '%s' is not in the index\n", path);
    return -1;
}

int index_status(const Index *index) {
    printf("Staged changes:\n");
    int staged_count = 0;
    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
        staged_count++;
    }
    if (staged_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    printf("Unstaged changes:\n");
    int unstaged_count = 0;
    for (int i = 0; i < index->count; i++) {
        struct stat st;
        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            unstaged_count++;
        } else {
            if (st.st_mtime != (time_t)index->entries[i].mtime_sec ||
                st.st_size != (off_t)index->entries[i].size) {
                printf("  modified:   %s\n", index->entries[i].path);
                unstaged_count++;
            }
        }
    }
    if (unstaged_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    printf("Untracked files:\n");
    int untracked_count = 0;
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if (strcmp(ent->d_name, ".pes") == 0) continue;
            if (strcmp(ent->d_name, "pes") == 0) continue;
            if (strstr(ent->d_name, ".o") != NULL) continue;

            int is_tracked = 0;
            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    is_tracked = 1;
                    break;
                }
            }

            if (!is_tracked) {
                struct stat st;
                stat(ent->d_name, &st);
                if (S_ISREG(st.st_mode)) {
                    printf("  untracked:  %s\n", ent->d_name);
                    untracked_count++;
                }
            }
        }
        closedir(dir);
    }
    if (untracked_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    return 0;
}

// ─── IMPLEMENTATION ─────────────────────────────────────────────────────────

// Helper for sorting
static int compare_entries(const void *a, const void *b) {
    const IndexEntry *ea = a;
    const IndexEntry *eb = b;
    return strcmp(ea->path, eb->path);
}

// Load index
int index_load(Index *index) {
    index->count = 0;

    FILE *fp = fopen(".pes/index", "r");
    if (!fp) return 0; // empty index if not exists

    char path[1024];
    char hash_hex[65];
    unsigned int mode;
    long mtime;
    long size;

    while (fscanf(fp, "%o %64s %ld %ld %s\n",
                  &mode, hash_hex, &mtime, &size, path) == 5) {

        IndexEntry *e = &index->entries[index->count++];

        e->mode = mode;
        e->mtime_sec = mtime;
        e->size = size;
        strcpy(e->path, path);

        hex_to_hash(hash_hex, &e->hash);
    }

    fclose(fp);
    return 0;
}

// Save index
int index_save(const Index *index) {
    IndexEntry *sorted = malloc(index->count * sizeof(IndexEntry));
    if (!sorted) return -1;

    memcpy(sorted, index->entries, index->count * sizeof(IndexEntry));
    qsort(sorted, index->count, sizeof(IndexEntry), compare_entries);

    FILE *fp = fopen(".pes/index.tmp", "w");
    if (!fp) {
        free(sorted);
        return -1;
    }

    for (int i = 0; i < index->count; i++) {
        char hash_hex[65];
        hash_to_hex(&sorted[i].hash, hash_hex);

        fprintf(fp, "%o %s %ld %ld %s\n",
                sorted[i].mode,
                hash_hex,
                sorted[i].mtime_sec,
                sorted[i].size,
                sorted[i].path);
    }

    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);

    if (rename(".pes/index.tmp", ".pes/index") != 0) {
        free(sorted);
        return -1;
    }

    free(sorted);
    return 0;
}

// Add file to index
int index_add(Index *index, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    unsigned char *buffer = malloc(size);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    fread(buffer, 1, size, fp);
    fclose(fp);

    ObjectID hash;
    if (object_write(OBJ_BLOB, buffer, size, &hash) != 0) {
        free(buffer);
        return -1;
    }

    free(buffer);

    struct stat st;
    if (stat(path, &st) != 0) {
        perror("stat");
        return -1;
    }

    IndexEntry *entry = index_find(index, path);

    if (entry) {
        entry->mode = st.st_mode;
        entry->mtime_sec = st.st_mtime;
        entry->size = st.st_size;
        entry->hash = hash;
    } else {
        entry = &index->entries[index->count++];
        entry->mode = st.st_mode;
        entry->mtime_sec = st.st_mtime;
        entry->size = st.st_size;
        entry->hash = hash;
        strcpy(entry->path, path);
    }

    return index_save(index);
}
